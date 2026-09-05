/* Contrat du registre J : valeurs refusées, chaîne canonique complète et
 * invalidation du cache de seuils de capitale. Banc autonome, sans monde ni save. */
#include "scps_tune.h"
#include "scps_labor.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int g_checks=0, g_passes=0;
static int check(int ok, const char *what){
    ++g_checks; if (ok) ++g_passes;
    if (!ok) fprintf(stderr, "tune_contract: ECHEC %s\n", what);
    return ok;
}

int main(void){
    int pass=1;
    tune_init();
    unsigned r0=tune_revision();
    pass &= check(tune_f(NULL, 123.f)==123.f, "fallback nom NULL");
    pass &= check(!tune_set_checked(NULL, 1.f) && tune_phase(NULL)==NULL, "API NULL sûr");
    float old=tune_f("TIER2_POP", 2000.f);
    pass &= check(!tune_set_checked("UNKNOWN_KEY", 1.f), "nom inconnu refusé");
    pass &= check(!tune_set_checked("TIER2_POP", NAN), "NaN refusé");
    pass &= check(!tune_set_checked("TIER2_POP", INFINITY), "Infini refusé");
    pass &= check(tune_f("TIER2_POP", 2000.f)==old, "refus sans mutation");
    pass &= check(!tune_is_active("REGION_RAW_KEEP"), "clé legacy inactive");
    pass &= check(!tune_set_checked("REGION_RAW_KEEP", 1.f), "clé inactive refusée");
    pass &= check(strcmp(tune_phase("REGION_RAW_KEEP"), "inactive")==0, "phase inactive");
    pass &= check(strcmp(tune_phase("TOPONYM_RIVER_MAJOR"), "new_world")==0, "phase nouveau monde");
    pass &= check(strcmp(tune_phase("INVARIANT_SCALE_FLOOR"), "diagnostic")==0, "phase diagnostic");
    pass &= check(!tune_set_checked("RELIG_SCHISM_MAX", -1.f), "borne schisme");
    pass &= check(!tune_set_checked("RELIG_SCHOLAR_DAYS", 0.f), "borne durée religion");
    pass &= check(!tune_set_checked("MANUF_BUILD_COST", -1.f), "coût négatif refusé");
    pass &= check(tune_revision()==r0, "révision stable après refus");

    pass &= check(tune_set_checked("TIER2_POP", 1000.0001f), "flottant précis A");
    uint32_t fpa=tune_fingerprint32();
    pass &= check(tune_set_checked("TIER2_POP", 1000.0002f), "flottant précis B");
    pass &= check(tune_fingerprint32()!=fpa, "empreinte bits flottants");

    /* Remplir le registre crée une chaîne > 1023 octets. Une clé en fin de
     * registre doit encore modifier son empreinte : aucun préfixe tronqué. */
    for (int i=0;i<tune_count();i++){
        const char *name=tune_name_at(i);
        if (tune_is_active(name))
            pass &= check(tune_set_checked(name, tune_value_at(i)+1.f), "surcharge connue");
    }
    size_t n=strlen(tune_active_string());
    pass &= check(n>1023, "chaîne active complète");
    uint32_t fp=tune_fingerprint32();
    int last=-1;
    for (int i=tune_count()-1;i>=0;i--) if (tune_is_active(tune_name_at(i))){ last=i; break; }
    if (last>=0){
        const char *name=tune_name_at(last);
        pass &= check(tune_set_checked(name, tune_value_at(last)+0.125f), "clé tardive acceptée");
        pass &= check(tune_fingerprint32()!=fp, "empreinte complète sensible à la fin");
    }

    /* Le seuil T2 est mis en cache pour le tick, mais la révision le rafraîchit
     * après une édition F10/API. */
    pass &= check(tune_set_checked("TIER2_POP", 2000.f), "seuil haut accepté");
    pass &= check(capitale_max_tier(1500)==1, "cache seuil haut");
    pass &= check(tune_set_checked("TIER2_POP", 500.f), "seuil bas accepté");
    pass &= check(capitale_max_tier(1500)>=2, "cache rafraîchi");
    pass &= check(strcmp(tune_phase("TIER2_POP"), "rule_read")==0, "phase lecture règle");
    pass &= check(tune_reset("TIER2_POP"), "reset accepté");
    pass &= check(tune_overridden_at(0) || tune_overridden_at(1), "métadonnée surcharge disponible");
    printf("tune_contract_demo: %s (%d clés, %zu octets, revision %u)\n",
           pass ? "PASS" : "FAIL", tune_count(), strlen(tune_active_string()), tune_revision());
    printf("BILAN %d/%d\n", g_passes, g_checks);
    return pass ? 0 : 1;
}
