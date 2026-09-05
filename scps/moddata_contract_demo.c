/* Contract checks for the optional SCPS_MODS loaders.
 * The fixture is written below TMPDIR and removed before returning. */
#include "scps_econ.h"
#include "scps_army.h"
#include "scps_tech.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures, checks;
static void check(int condition, const char *what){
    checks++;
    if (condition) printf("PASS %s\n", what);
    else { printf("FAIL %s\n", what); failures++; }
}

static int dump_contains(void (*dump)(FILE *), const char *needle){
    char buf[32768]; FILE *f=tmpfile();
    if(!f) return 0;
    dump(f); fflush(f); rewind(f);
    size_t n=fread(buf,1,sizeof(buf)-1,f); buf[n]=0;
    fclose(f);
    return strstr(buf,needle)!=NULL;
}

int main(void){
    const char *tmp=getenv("TMPDIR"); if(!tmp||!*tmp) tmp=".";
    char path[512];
    snprintf(path,sizeof path,"%s/scps_moddata_contract_demo.tsv",tmp);
    FILE *f=fopen(path,"w");
    if(!f){ fprintf(stderr,"cannot create fixture %s\n",path); return 2; }
    fputs("# valid rows are committed; malformed rows are rejected atomically\n",f);
    for(int i=0;i<300;i++) fputc('x',f);
    fputc('\n',f);                              /* overlong physical line */
    fputs("price\tBois\t2.5\t1.25\n",f);
    fputs("price\tBois\tnan\t9\n",f);
    fputs("price\tBois\t9\tinf\n",f);
    fputs("price\tFer\tnan\t0.4\n",f);          /* invalid-only: stays at default */
    fputs("price\tBois\tbad\t1\n",f);
    fputs("price\tBois\t3\t-1\n",f);            /* negative production is invalid */
    fputs("price\tCuivre\t0.5\n",f);             /* legacy price-only row remains valid */
    fputs("recipe\tScierie navale\t2.25\t1.75\n",f);
    fputs("recipe\tScierie navale\tbad\t8\n",f);
    fputs("recipe\tScierie navale\t4\t0\n",f);  /* zero disables output safely */
    fputs("unit\tPiquier\t0.6\t120\t2.5\t4.5\n",f);
    fputs("unit\tPiquier\tnan\t130\t2\t4\n",f);
    fputs("unit\tArcher\t.2\t70\tinf\t3\n",f);  /* invalid-only */
    fputs("unit\tPiquier\t.7\t120\t-1\t4\n",f); /* negative movement is invalid */
    fputs("unit\tMilice\t0\t0\t0\t0\n",f);       /* finite zero safely disables a unit */
    fputs("basecost\t2\t222\n",f);
    fputs("basecost\t2\tNaN\n",f);
    fputs("basecost\t2\t-1\n",f);
    fputs("basecost\t2\t22junk\n",f);
    fputs("techbonus\tFonderie\t.42\t.13\n",f);
    fputs("techbonus\tFonderie\t.41\tNaN\n",f);
    fputs("techbonus\tCommerce\tnan\t.1\n",f);   /* invalid-only: stays at default */
    fputs("techbonus\tFonderie\t.4\t-1\n",f);
    fputs("techbonus\tFonderie\t.4\t.2\textra\n",f);
    fclose(f);

    float fer_before=econ_base_price(RES_IRON);
    const UnitDef *arch_before=unit_def(U_ARCHER);
    float arch_mvt_before=arch_before?arch_before->mouvement:0.f;
    float commerce_before=tech_node_prod_pct(TECH_COMMERCE);

    int ne=econ_moddata_load(path);
    int na=army_moddata_load(path);
    int nt=tech_moddata_load(path);
    check(ne==4,"econ applies price-only, price, and recipe valid rows");
    check(na==2,"army applies one normal and one zero-valued valid unit row");
    check(nt==2,"tech applies basecost and techbonus valid rows");
    check(econ_base_price(RES_WOOD)==2.5f,"price valid value applied");
    check(econ_base_price(RES_COPPER)==0.5f,"legacy price-only row applies without changing yield");
    check(econ_base_price(RES_IRON)==fer_before,"invalid price leaves other resource unchanged");
    check(dump_contains(econ_moddata_dump,"price\tBois\t2.5\t1.25"),"extract yield commits with price atomically");
    check(dump_contains(econ_moddata_dump,"recipe\tScierie navale\t4\t0"),"recipe zero output is accepted as a safe disable");
    const UnitDef *pi=unit_def(U_PIQUIER), *arch=unit_def(U_ARCHER);
    check(pi && pi->discipline==0.6f && pi->moral==120.f && pi->mouvement==2.5f && pi->commandement==4.5f,
          "unit valid values applied together");
    check(arch && arch->mouvement==arch_mvt_before,"invalid unit leaves other unit unchanged");
    const UnitDef *milice=unit_def(U_MILICE);
    check(milice && milice->discipline==0.f && milice->moral==0.f &&
          milice->mouvement==0.f && milice->commandement==0.f,
          "zero unit stats are accepted as a safe disable");
    check(tech_node_prod_pct(TECH_FONDERIE)==0.42f && tech_node_eff_pct(TECH_FONDERIE)==0.13f,
          "tech bonus fields commit together");
    check(tech_node_prod_pct(TECH_COMMERCE)==commerce_before,"invalid tech bonus leaves other node unchanged");
    check(dump_contains(tech_moddata_dump,"basecost\t2\t222"),"tech base cost valid value applied");
    check(pi!=NULL,"unit definition remains available after loading");

    /* A real combined dump is the second fixture: each loader must consume its
     * own tags and silently skip the two foreign sections. */
    f=fopen(path,"w");
    if(!f){ fprintf(stderr,"cannot create roundtrip fixture %s\n",path); remove(path); return 2; }
    econ_moddata_dump(f); army_moddata_dump(f); tech_moddata_dump(f); fclose(f);
    int expected_e=0, expected_a=0, expected_t=0;
    char row[256];
    f=fopen(path,"r");
    if(!f){ remove(path); return 2; }
    while(fgets(row,sizeof row,f)){
        if(!strncmp(row,"price\t",6)||!strncmp(row,"recipe\t",7)) expected_e++;
        if(!strncmp(row,"unit\t",5)) expected_a++;
        if(!strncmp(row,"basecost\t",9)||!strncmp(row,"techbonus\t",10)) expected_t++;
    }
    fclose(f);
    int re=econ_moddata_load(path), ra=army_moddata_load(path), rt=tech_moddata_load(path);
    check(expected_e>0 && expected_a>0 && expected_t>0 &&
          re==expected_e && ra==expected_a && rt==expected_t,
          "combined dump reloads every record in its owning module");

    remove(path);
    printf("MODDATA_CONTRACT %d passed, %d failed\n", checks-failures, failures);
    return failures?1:0;
}
