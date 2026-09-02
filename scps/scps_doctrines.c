/*
 * scps_doctrines.c — LES DOCTRINES (cf. scps_doctrines.h pour la doctrine).
 *
 * Propriété golden forte : le module NE TIRE AUCUN xs32. Un pays sans doctrine
 * sort de doctrine_key_mult au PREMIER test, et avec AI_DOCT=0 aucun pays IA
 * n'en adopte jamais, donc TOUS les sites de lecture rendent exactement 1.0f
 * (multiplication IEEE neutre) et le golden est intact PAR CONSTRUCTION.
 *
 * v107 : AUCUN ENTRETIEN — les doctrines coûtent en FLAT (l'achat, rien
 * d'autre), la mécanique de suspension mensuelle a disparu avec lui. Il n'y a
 * donc plus de CLÔTURE MENSUELLE : le miroir de process ne bouge qu'aux
 * verbes (adopter/acheter/abandonner) et au chargement (doctrines_sync).
 */
#include "scps_doctrines.h"
#include "scps_influence.h"
#include "scps_lang.h"    /* StrId : les noms/bonus du catalogue naissent en STR_* */
#include "scps_tune.h"    /* registre J : DOCT_COST_* / IDEA_COST_* / INFLUENCE_PER_* */
#include <string.h>

/* LE CATALOGUE — 17 doctrines x 6 idees (docs/DESIGN_DOCTRINES_ANNEXE.md).
 * Une idee porte jusqu'a DEUX (cle de registre, multiplicateur) ; le SITE de
 * lecture moteur interroge la cle par doctrine_key_mult(cid, "CLE").
 *   verbe = l'idee debloque une ACTION (vague suivante : achetable, sans effet).
 *   cable = MASQUE (bit0 : k1 portee au site · bit1 : k2 portee au site). 0 =
 *           aucun effet moteur cette vague (cle fantome du registre, site
 *           hoiste hors de portee d'un cid, pure regle, ou VERBE) : la cle
 *           reste ecrite pour memoire, la facade l'affiche << a venir >>. */
const DoctDef DOCT_DEF[DOCT_COUNT] = {
  { STR_DOCT_OFFENSE_NAME, STR_DOCT_OFFENSE_HOVER, "doct_offense_bg", {
    { STR_IDEA_OFFENSE_ARSENAUX_NAME, STR_IDEA_OFFENSE_ARSENAUX_BONUS, "idea_offense_arsenaux", "ARMS_PER_LABORER", 1.2500f, "ARSENAL_DECAY", 1.0050f, 0, 3 },
    { STR_IDEA_OFFENSE_DISCIPLINE_NAME, STR_IDEA_OFFENSE_DISCIPLINE_BONUS, "idea_offense_discipline", "BT_DMG_K", 1.1000f, "CTR_BITE", 1.1500f, 0, 3 },
    { STR_IDEA_OFFENSE_OST_NAME, STR_IDEA_OFFENSE_OST_BONUS, "idea_offense_ost", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_OFFENSE_BUTIN_NAME, STR_IDEA_OFFENSE_BUTIN_BONUS, "idea_offense_butin", "SIEGE_LOOT_FRAC", 1.3000f, "PILLAGE_INCOME_FRAC", 1.1500f, 0, 0 },
    { STR_IDEA_OFFENSE_PRETEXTES_NAME, STR_IDEA_OFFENSE_PRETEXTES_BONUS, "idea_offense_pretextes", "FAB_CB_COST_YEARS", 0.6000f, "FAB_MATURE_DAYS", 0.7000f, 0, 0 },
    { STR_IDEA_OFFENSE_LEVEE_NAME, STR_IDEA_OFFENSE_LEVEE_BONUS, "idea_offense_levee", "SOLDE_FL_PER_REG", 1.3000f, "SOLDE_OVER_K", 0.8000f, 0, 0 },
  }},
  { STR_DOCT_DEFENSE_NAME, STR_DOCT_DEFENSE_HOVER, "doct_defense_bg", {
    { STR_IDEA_DEFENSE_REMPARTS_NAME, STR_IDEA_DEFENSE_REMPARTS_BONUS, "idea_defense_remparts", "DEF_PER_H", 1.3000f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_DEFENSE_MAGASINS_NAME, STR_IDEA_DEFENSE_MAGASINS_BONUS, "idea_defense_magasins", "SIEGE_FOOD_MONTHS_FULL", 1.2500f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_DEFENSE_BAN_NAME, STR_IDEA_DEFENSE_BAN_BONUS, "idea_defense_ban", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_DEFENSE_CORVEES_NAME, STR_IDEA_DEFENSE_CORVEES_BONUS, "idea_defense_corvees", "BUILD_COST_MULT_FORT", 0.8000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_DEFENSE_TERRE_BRULEE_NAME, STR_IDEA_DEFENSE_TERRE_BRULEE_BONUS, "idea_defense_terre_brulee", "SIEGE_LOOT_FRAC", 0.6000f, "PILLAGE_INCOME_FRAC", 0.8000f, 0, 3 },
    { STR_IDEA_DEFENSE_GENIE_NAME, STR_IDEA_DEFENSE_GENIE_BONUS, "idea_defense_genie", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
  }},
  { STR_DOCT_COMMERCE_NAME, STR_DOCT_COMMERCE_HOVER, "doct_commerce_bg", {
    { STR_IDEA_COMMERCE_FRANCHISES_NAME, STR_IDEA_COMMERCE_FRANCHISES_BONUS, "idea_commerce_franchises", "TRADE_LEVY", 0.7500f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_COMMERCE_ROUTES_LONGUES_NAME, STR_IDEA_COMMERCE_ROUTES_LONGUES_BONUS, "idea_commerce_routes_longues", "MARKET_DIST_FALLOFF", 0.8000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_COMMERCE_COMPTOIR_NAME, STR_IDEA_COMMERCE_COMPTOIR_BONUS, "idea_commerce_comptoir", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_COMMERCE_NEGOCE_NAME, STR_IDEA_COMMERCE_NEGOCE_BONUS, "idea_commerce_negoce", "IMPORT_MARGIN_THIRD", 0.8500f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_COMMERCE_GUILDES_NAME, STR_IDEA_COMMERCE_GUILDES_BONUS, "idea_commerce_guildes", "COMMERCE_W_BOURGEOIS", 1.3000f, "COMMERCE_W_ELITE", 0.8000f, 0, 3 },
    { STR_IDEA_COMMERCE_LIBRE_ECHANGE_NAME, STR_IDEA_COMMERCE_LIBRE_ECHANGE_BONUS, "idea_commerce_libre_echange", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
  }},
  { STR_DOCT_MERCANTILISME_NAME, STR_DOCT_MERCANTILISME_HOVER, "doct_mercantilisme_bg", {
    { STR_IDEA_MERCANTILISME_RESERVES_NAME, STR_IDEA_MERCANTILISME_RESERVES_BONUS, "idea_mercantilisme_reserves", "BUILD_RESERVE_BULK", 1.3000f, "AI_SAFE_STOCK_MONTHS", 1.3000f, 0, 2 },
    { STR_IDEA_MERCANTILISME_REGIE_NAME, STR_IDEA_MERCANTILISME_REGIE_BONUS, "idea_mercantilisme_regie", "SPEC_BUY_BAND", 1.1000f, "SPEC_SELL_BAND", 0.9200f, 0, 3 },
    { STR_IDEA_MERCANTILISME_BLOCUS_NAME, STR_IDEA_MERCANTILISME_BLOCUS_BONUS, "idea_mercantilisme_blocus", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_MERCANTILISME_ETAPE_NAME, STR_IDEA_MERCANTILISME_ETAPE_BONUS, "idea_mercantilisme_etape", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_MERCANTILISME_PEAGES_NAME, STR_IDEA_MERCANTILISME_PEAGES_BONUS, "idea_mercantilisme_peages", "IMPORT_MARGIN_OWN", 0.8000f, "TOLL_STATE_SHARE", 1.5000f, 0, 0 },
    { STR_IDEA_MERCANTILISME_HALLES_NAME, STR_IDEA_MERCANTILISME_HALLES_BONUS, "idea_mercantilisme_halles", "STOCK_CAP_ENTREPOT", 1.3000f, "STOCK_DECAY_PERISH", 1.0600f, 0, 0 },
  }},
  { STR_DOCT_PEUPLE_NAME, STR_DOCT_PEUPLE_HOVER, "doct_peuple_bg", {
    { STR_IDEA_PEUPLE_ACCUEIL_NAME, STR_IDEA_PEUPLE_ACCUEIL_BONUS, "idea_peuple_accueil", "AI_OFFER_MIG_OPINION", 0.7000f, "MIG_PACT_MIN", 0.5000f, 0, 1 },
    { STR_IDEA_PEUPLE_ECOLES_NAME, STR_IDEA_PEUPLE_ECOLES_BONUS, "idea_peuple_ecoles", "ASSIM_K_INST_REF", 0.8000f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_PEUPLE_ASILE_NAME, STR_IDEA_PEUPLE_ASILE_BONUS, "idea_peuple_asile", "REFUGEE_SETTLE_INTEG", 0.9000f, "REFUGEE_RETURN_PULL", 0.8000f, 0, 0 },
    { STR_IDEA_PEUPLE_AFFRANCHISSEMENT_NAME, STR_IDEA_PEUPLE_AFFRANCHISSEMENT_BONUS, "idea_peuple_affranchissement", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_PEUPLE_TOLERANCE_NAME, STR_IDEA_PEUPLE_TOLERANCE_BONUS, "idea_peuple_tolerance", "OFF_CULTURE_SAT_PEN", 0.7500f, "OFF_CULTURE_SOC_PEN", 0.7500f, 0, 0 },
    { STR_IDEA_PEUPLE_METISSAGE_NAME, STR_IDEA_PEUPLE_METISSAGE_BONUS, "idea_peuple_metissage", "METAB_TIER12", 0.7500f, "AI_METAB_RES_W", 1.2000f, 0, 2 },
  }},
  { STR_DOCT_COLONISATION_NAME, STR_DOCT_COLONISATION_HOVER, "doct_colonisation_bg", {
    { STR_IDEA_COLONISATION_COLONS_NAME, STR_IDEA_COLONISATION_COLONS_BONUS, "idea_colonisation_colons", "COLONY_MIN_POP", 0.8500f, "COLONY_COST_POP", 0.9000f, 0, 0 },
    { STR_IDEA_COLONISATION_RAVITAILLEMENT_NAME, STR_IDEA_COLONISATION_RAVITAILLEMENT_BONUS, "idea_colonisation_ravitaillement", "FOOD_STOCK_MONTHS", 0.7500f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_COLONISATION_ACCLIMATATION_NAME, STR_IDEA_COLONISATION_ACCLIMATATION_BONUS, "idea_colonisation_acclimatation", "HAB_MALUS_K", 0.8000f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_COLONISATION_DOUBLE_CHANTIER_NAME, STR_IDEA_COLONISATION_DOUBLE_CHANTIER_BONUS, "idea_colonisation_double_chantier", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_COLONISATION_CLIMATS_NAME, STR_IDEA_COLONISATION_CLIMATS_BONUS, "idea_colonisation_climats", "CLIM_LEARN_INTEG", 0.9000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_COLONISATION_GRAND_LARGE_NAME, STR_IDEA_COLONISATION_GRAND_LARGE_BONUS, "idea_colonisation_grand_large", "COLONY_YIELD_HREF", 1.5000f, NULL, 0.0000f, 0, 0 },
  }},
  { STR_DOCT_DIPLOMATIE_NAME, STR_DOCT_DIPLOMATIE_HOVER, "doct_diplomatie_bg", {
    { STR_IDEA_DIPLOMATIE_PRESTIGE_NAME, STR_IDEA_DIPLOMATIE_PRESTIGE_BONUS, "idea_diplomatie_prestige", "OPINION_ALLY", 1.2500f, "OPINION_PACT", 1.2500f, 0, 3 },
    { STR_IDEA_DIPLOMATIE_CHANCELLERIE_NAME, STR_IDEA_DIPLOMATIE_CHANCELLERIE_BONUS, "idea_diplomatie_chancellerie", "INFLUENCE_COST_ENVOY", 0.8000f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_DIPLOMATIE_OUBLI_NAME, STR_IDEA_DIPLOMATIE_OUBLI_BONUS, "idea_diplomatie_oubli", "OPINION_MEM_DECAY", 1.3000f, "OPINION_RANCOR_W", 0.8500f, 0, 0 },
    { STR_IDEA_DIPLOMATIE_SECOND_EMISSAIRE_NAME, STR_IDEA_DIPLOMATIE_SECOND_EMISSAIRE_BONUS, "idea_diplomatie_second_emissaire", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_DIPLOMATIE_PERSUASION_NAME, STR_IDEA_DIPLOMATIE_PERSUASION_BONUS, "idea_diplomatie_persuasion", "AI_OFFER_ALLY_OPINION", 0.7500f, "AI_OFFER_MIG_OPINION", 0.8000f, 0, 3 },
    { STR_IDEA_DIPLOMATIE_CONGRES_NAME, STR_IDEA_DIPLOMATIE_CONGRES_BONUS, "idea_diplomatie_congres", "AI_WAR_EXHAUST", 0.7500f, "AI_WAR_DECISIVE", 0.8500f, 0, 0 },
  }},
  { STR_DOCT_VASSAUX_NAME, STR_DOCT_VASSAUX_HOVER, "doct_vassaux_bg", {
    { STR_IDEA_VASSAUX_SERMENTS_NAME, STR_IDEA_VASSAUX_SERMENTS_BONUS, "idea_vassaux_serments", "AI_VASSAL_INTEGRATE_YEARS", 0.8500f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_VASSAUX_TRIBUT_VASSAL_NAME, STR_IDEA_VASSAUX_TRIBUT_VASSAL_BONUS, "idea_vassaux_tribut_vassal", "AI_VASSAL_CONTRIB_GATE", 0.8500f, "AI_VASSAL_CONTRIB_BASE", 1.2000f, 0, 3 },
    { STR_IDEA_VASSAUX_CONTRATS_NAME, STR_IDEA_VASSAUX_CONTRATS_BONUS, "idea_vassaux_contrats", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_VASSAUX_LEVIERS_NAME, STR_IDEA_VASSAUX_LEVIERS_BONUS, "idea_vassaux_leviers", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_VASSAUX_ANNEXION_NAME, STR_IDEA_VASSAUX_ANNEXION_BONUS, "idea_vassaux_annexion", "ANNEX_INTEGRATION_DISCOUNT", 1.2500f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_VASSAUX_SUZERAINETE_NAME, STR_IDEA_VASSAUX_SUZERAINETE_BONUS, "idea_vassaux_suzerainete", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
  }},
  { STR_DOCT_PRODUCTION_NAME, STR_DOCT_PRODUCTION_HOVER, "doct_production_bg", {
    { STR_IDEA_PRODUCTION_EXTRACTION_NAME, STR_IDEA_PRODUCTION_EXTRACTION_BONUS, "idea_production_extraction", "EXTRACT_LABOR_SHARE", 1.1200f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_PRODUCTION_OUTILLAGE_NAME, STR_IDEA_PRODUCTION_OUTILLAGE_BONUS, "idea_production_outillage", "TOOLS_PER_LABORER", 1.3000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_PRODUCTION_EXPLOITATION_NAME, STR_IDEA_PRODUCTION_EXPLOITATION_BONUS, "idea_production_exploitation", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_PRODUCTION_MANUFACTURES_NAME, STR_IDEA_PRODUCTION_MANUFACTURES_BONUS, "idea_production_manufactures", "MANUF_QOUT_MULT", 1.1500f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_PRODUCTION_GAGES_NAME, STR_IDEA_PRODUCTION_GAGES_BONUS, "idea_production_gages", "MANUF_BUILD_COST", 0.8500f, "JOB_UPKEEP_TAX_FRAC", 0.8000f, 0, 1 },
    { STR_IDEA_PRODUCTION_RENDEMENT_NAME, STR_IDEA_PRODUCTION_RENDEMENT_BONUS, "idea_production_rendement", "RAW_BOOST_PER_TIER", 1.2000f, "RAW_BOOST_COST", 0.7500f, 0, 1 },
  }},
  { STR_DOCT_INFRASTRUCTURE_NAME, STR_DOCT_INFRASTRUCTURE_HOVER, "doct_infrastructure_bg", {
    { STR_IDEA_INFRASTRUCTURE_MACONS_NAME, STR_IDEA_INFRASTRUCTURE_MACONS_BONUS, "idea_infrastructure_macons", "BUILD_MAT_MULT", 0.9000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_INFRASTRUCTURE_CARRIERES_NAME, STR_IDEA_INFRASTRUCTURE_CARRIERES_BONUS, "idea_infrastructure_carrieres", "BUILD_RESERVE_BULK", 1.3000f, "RAW_WORKS_NEED", 1.3000f, 0, 2 },
    { STR_IDEA_INFRASTRUCTURE_ENTRETIEN_NAME, STR_IDEA_INFRASTRUCTURE_ENTRETIEN_BONUS, "idea_infrastructure_entretien", "VETUSTE_RATE", 0.7500f, "VETUSTE_FLOOR", 1.1500f, 0, 0 },
    { STR_IDEA_INFRASTRUCTURE_RENOVATION_NAME, STR_IDEA_INFRASTRUCTURE_RENOVATION_BONUS, "idea_infrastructure_renovation", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_INFRASTRUCTURE_LOGEMENTS_NAME, STR_IDEA_INFRASTRUCTURE_LOGEMENTS_BONUS, "idea_infrastructure_logements", "HOUSE_MANUF", 1.2500f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_INFRASTRUCTURE_INTENDANCE_NAME, STR_IDEA_INFRASTRUCTURE_INTENDANCE_BONUS, "idea_infrastructure_intendance", "BUILD_EXTENT_K", 0.7000f, NULL, 0.0000f, 0, 0 },
  }},
  { STR_DOCT_TECHNOLOGIE_NAME, STR_DOCT_TECHNOLOGIE_HOVER, "doct_technologie_bg", {
    { STR_IDEA_TECHNOLOGIE_BIBLIOTHEQUES_NAME, STR_IDEA_TECHNOLOGIE_BIBLIOTHEQUES_BONUS, "idea_technologie_bibliotheques", "SAVOIR_LIB_PER", 1.2500f, "SAVOIR_LIB_MAX", 1.3000f, 0, 3 },
    { STR_IDEA_TECHNOLOGIE_ECOLES_VILLE_NAME, STR_IDEA_TECHNOLOGIE_ECOLES_VILLE_BONUS, "idea_technologie_ecoles_ville", "SAVOIR_W_BOURGEOIS", 1.3000f, "SAVOIR_W_LABORER", 1.2500f, 0, 3 },
    { STR_IDEA_TECHNOLOGIE_PROGRAMME_NAME, STR_IDEA_TECHNOLOGIE_PROGRAMME_BONUS, "idea_technologie_programme", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_TECHNOLOGIE_COPISTES_NAME, STR_IDEA_TECHNOLOGIE_COPISTES_BONUS, "idea_technologie_copistes", "AI_TECH_DIFFUSE_MAX", 1.3000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_TECHNOLOGIE_DISPENSE_NAME, STR_IDEA_TECHNOLOGIE_DISPENSE_BONUS, "idea_technologie_dispense", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_TECHNOLOGIE_SOBRIETE_NAME, STR_IDEA_TECHNOLOGIE_SOBRIETE_BONUS, "idea_technologie_sobriete", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
  }},
  { STR_DOCT_CONNAISSANCES_NAME, STR_DOCT_CONNAISSANCES_HOVER, "doct_connaissances_bg", {
    { STR_IDEA_CONNAISSANCES_PORTULANS_NAME, STR_IDEA_CONNAISSANCES_PORTULANS_BONUS, "idea_connaissances_portulans", "FOG_SEA_HALO", 2.0000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_CONNAISSANCES_TRUCHEMENTS_NAME, STR_IDEA_CONNAISSANCES_TRUCHEMENTS_BONUS, "idea_connaissances_truchements", "SYNC_TRADE_METIER", 0.8000f, "SYNC_TRADE_PROFOND", 0.8000f, 0, 3 },
    { STR_IDEA_CONNAISSANCES_EXPEDITION_NAME, STR_IDEA_CONNAISSANCES_EXPEDITION_BONUS, "idea_connaissances_expedition", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_CONNAISSANCES_DICTIONNAIRES_NAME, STR_IDEA_CONNAISSANCES_DICTIONNAIRES_BONUS, "idea_connaissances_dictionnaires", "METAB_TIER12", 0.8000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_CONNAISSANCES_COLLEGES_NAME, STR_IDEA_CONNAISSANCES_COLLEGES_BONUS, "idea_connaissances_colleges", "AI_METAB_RES_W", 1.4000f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_CONNAISSANCES_LANGUE_FRANQUE_NAME, STR_IDEA_CONNAISSANCES_LANGUE_FRANQUE_BONUS, "idea_connaissances_langue_franque", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
  }},
  { STR_DOCT_FAUSTIEN_NAME, STR_DOCT_FAUSTIEN_HOVER, "doct_faustien_bg", {
    { STR_IDEA_FAUSTIEN_PAGES_INTERDITES_NAME, STR_IDEA_FAUSTIEN_PAGES_INTERDITES_BONUS, "idea_faustien_pages_interdites", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_FAUSTIEN_CREUSETS_NAME, STR_IDEA_FAUSTIEN_CREUSETS_BONUS, "idea_faustien_creusets", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_FAUSTIEN_PACTE_NAME, STR_IDEA_FAUSTIEN_PACTE_BONUS, "idea_faustien_pacte", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_FAUSTIEN_OR_DU_PUITS_NAME, STR_IDEA_FAUSTIEN_OR_DU_PUITS_BONUS, "idea_faustien_or_du_puits", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_FAUSTIEN_TERRE_CHANGEE_NAME, STR_IDEA_FAUSTIEN_TERRE_CHANGEE_BONUS, "idea_faustien_terre_changee", "FAUST_MUTATION_K", 1.2500f, "CHARGE_DECAY", 0.6500f, 0, 3 },
    { STR_IDEA_FAUSTIEN_PRIX_CONSENTI_NAME, STR_IDEA_FAUSTIEN_PRIX_CONSENTI_BONUS, "idea_faustien_prix_consenti", "FAUST_YIELD_MULT", 1.2500f, "FAUST_SPAWN_CHARGE", 1.5000f, 0, 3 },
  }},
  { STR_DOCT_ARISTOCRATIE_NAME, STR_DOCT_ARISTOCRATIE_HOVER, "doct_aristocratie_bg", {
    { STR_IDEA_ARISTOCRATIE_BANNERETS_NAME, STR_IDEA_ARISTOCRATIE_BANNERETS_BONUS, "idea_aristocratie_bannerets", "AI_VASSAL_CONTRIB_BASE", 1.2500f, "AI_VASSAL_CONTRIB_GATE", 0.8500f, 0, 3 },
    { STR_IDEA_ARISTOCRATIE_OFFICES_NAME, STR_IDEA_ARISTOCRATIE_OFFICES_BONUS, "idea_aristocratie_offices", "COUNCIL_PAY_ADJ", 1.3000f, "COUNCIL_DISMISS_GRIEF", 1.5000f, 0, 3 },
    { STR_IDEA_ARISTOCRATIE_ADOUBEMENT_NAME, STR_IDEA_ARISTOCRATIE_ADOUBEMENT_BONUS, "idea_aristocratie_adoubement", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_ARISTOCRATIE_FIEFS_NAME, STR_IDEA_ARISTOCRATIE_FIEFS_BONUS, "idea_aristocratie_fiefs", "EDI_ELITE_JOBS", 1.3500f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_ARISTOCRATIE_BAN_FEODAL_NAME, STR_IDEA_ARISTOCRATIE_BAN_FEODAL_BONUS, "idea_aristocratie_ban_feodal", "MORAL_MUL", 1.1500f, "INCOME_TAX_RATE_ELITE", 0.7500f, 0, 0 },
    { STR_IDEA_ARISTOCRATIE_CLOTURE_NAME, STR_IDEA_ARISTOCRATIE_CLOTURE_BONUS, "idea_aristocratie_cloture", "PROMOTE_BASKET_MULT_ELITE", 0.7500f, "PROMOTE_BASKET_MULT", 1.3000f, 0, 0 },
  }},
  { STR_DOCT_BOURGEOISIE_NAME, STR_DOCT_BOURGEOISIE_HOVER, "doct_bourgeoisie_bg", {
    { STR_IDEA_BOURGEOISIE_CHARTES_NAME, STR_IDEA_BOURGEOISIE_CHARTES_BONUS, "idea_bourgeoisie_chartes", "ADMIN_BASE", 0.8500f, NULL, 0.0000f, 0, 1 },
    { STR_IDEA_BOURGEOISIE_JURANDES_NAME, STR_IDEA_BOURGEOISIE_JURANDES_BONUS, "idea_bourgeoisie_jurandes", "COMMERCE_W_BOURGEOIS", 1.2000f, "PROMOTE_BASKET_MULT", 1.1000f, 0, 1 },
    { STR_IDEA_BOURGEOISIE_EMPRUNT_NAME, STR_IDEA_BOURGEOISIE_EMPRUNT_BONUS, "idea_bourgeoisie_emprunt", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_BOURGEOISIE_CREDIT_NAME, STR_IDEA_BOURGEOISIE_CREDIT_BONUS, "idea_bourgeoisie_credit", "CREDIT_RATE_BASE", 0.8000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_BOURGEOISIE_ROBE_NAME, STR_IDEA_BOURGEOISIE_ROBE_BONUS, "idea_bourgeoisie_robe", "COUNCIL_ROT_BOOST", 1.2500f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_BOURGEOISIE_CLES_DE_LA_VILLE_NAME, STR_IDEA_BOURGEOISIE_CLES_DE_LA_VILLE_BONUS, "idea_bourgeoisie_cles_de_la_ville", "PROMOTE_BASKET_MULT", 0.7500f, NULL, 0.0000f, 0, 0 },
  }},
  { STR_DOCT_POPULAIRE_NAME, STR_DOCT_POPULAIRE_HOVER, "doct_populaire_bg", {
    { STR_IDEA_POPULAIRE_DOLEANCES_NAME, STR_IDEA_POPULAIRE_DOLEANCES_BONUS, "idea_populaire_doleances", "POL_SAT_W", 1.2000f, "W_AGITATION_UNREST", 0.8500f, 0, 3 },
    { STR_IDEA_POPULAIRE_PAIN_NAME, STR_IDEA_POPULAIRE_PAIN_BONUS, "idea_populaire_pain", "TAX_EXEMPT_BASKET_MULT", 1.3000f, "POP_SAT_W", 1.2500f, 0, 3 },
    { STR_IDEA_POPULAIRE_LEVEE_EN_MASSE_NAME, STR_IDEA_POPULAIRE_LEVEE_EN_MASSE_BONUS, "idea_populaire_levee_en_masse", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_POPULAIRE_CONCESSION_NAME, STR_IDEA_POPULAIRE_CONCESSION_BONUS, "idea_populaire_concession", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_POPULAIRE_IMPOT_DU_RANG_NAME, STR_IDEA_POPULAIRE_IMPOT_DU_RANG_BONUS, "idea_populaire_impot_du_rang", "INCOME_TAX_RATE_ELITE", 1.2000f, "EDI_ELITE_POP_PCT", 0.8500f, 0, 2 },
    { STR_IDEA_POPULAIRE_SOUVERAINETE_NAME, STR_IDEA_POPULAIRE_SOUVERAINETE_BONUS, "idea_populaire_souverainete", "C3_K_HOLLOW", 0.2500f, "C3_L_HOLLOW", 0.0000f, 0, 3 },
  }},
  { STR_DOCT_DIVIN_NAME, STR_DOCT_DIVIN_HOVER, "doct_divin_bg", {
    { STR_IDEA_DIVIN_ONCTION_NAME, STR_IDEA_DIVIN_ONCTION_BONUS, "idea_divin_onction", "LEGIT_K_FAITH", 1.2500f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_DIVIN_FERVEUR_NAME, STR_IDEA_DIVIN_FERVEUR_BONUS, "idea_divin_ferveur", "PROVMOD_FERVEUR_K", 1.2000f, "PROVMOD_FERVEUR_DECAY", 0.7500f, 0, 3 },
    { STR_IDEA_DIVIN_SACERDOCE_NAME, STR_IDEA_DIVIN_SACERDOCE_BONUS, "idea_divin_sacerdoce", NULL, 0.0000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_DIVIN_APPEL_NAME, STR_IDEA_DIVIN_APPEL_BONUS, "idea_divin_appel", NULL, 0.0000f, NULL, 0.0000f, 1, 0 },
    { STR_IDEA_DIVIN_CLERGE_NAME, STR_IDEA_DIVIN_CLERGE_BONUS, "idea_divin_clerge", "SCHOLAR_DURATION_DAYS", 1.4000f, NULL, 0.0000f, 0, 0 },
    { STR_IDEA_DIVIN_ORTHODOXIE_NAME, STR_IDEA_DIVIN_ORTHODOXIE_BONUS, "idea_divin_orthodoxie", "AI_DERIVE_ODDS", 2.0000f, "RELIG_MINORITY_SAT", 1.6000f, 0, 3 },
  }},
};

/* ====================================================================== */
/* LE MIROIR DE PROCESS lu par doctrine_key_mult                          */
/* ====================================================================== */
/* Motif dessein_mult/decree_mult : les sites de lecture (econ, diplo, credit…)
 * n'ont pas le DoctrineState. On tient donc un MIROIR statique du nombre
 * d'idées EFFECTIVES par (pays, doctrine) — 0 si la doctrine n'est pas adoptée.
 * Le miroir n'est jamais sérialisé : il se reconstruit intégralement à chaque
 * verbe (adopter/acheter/abandonner) et au chargement (doctrines_sync). */
static int8_t g_own[SCPS_MAX_COUNTRY][DOCT_COUNT];
static int8_t g_any[SCPS_MAX_COUNTRY];   /* 1 = ce pays porte au moins une idée EFFECTIVE */

/* Cache par pays : la clé est un LITTÉRAL au site d'appel, donc la comparaison
 * de POINTEUR suffit presque toujours (strcmp en filet). Invalidé en bloc à
 * chaque resynchronisation du miroir — donc à chaque achat/abandon.
 * Déterministe : le cache ne fait que mémoriser un produit déjà calculé. */
#define DOCT_CACHE 16
static struct { const char *k; float m; } g_cache[SCPS_MAX_COUNTRY][DOCT_CACHE];
static uint8_t g_cn[SCPS_MAX_COUNTRY];

static void doct_cache_clear(void){
    memset(g_cache, 0, sizeof g_cache);
    memset(g_cn,    0, sizeof g_cn);
}

void doctrines_sync(const DoctrineState *ds){
    memset(g_own, 0, sizeof g_own);
    memset(g_any, 0, sizeof g_any);
    doct_cache_clear();
    if (!ds) return;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        for (int s=0;s<DOCT_SLOTS_MAX;s++){
            int d = ds->doct[c][s];
            if (d<0 || d>=DOCT_COUNT) continue;
            int n = ds->ideas[c][d];
            if (n<0) n=0;
            if (n>DOCT_IDEAS) n=DOCT_IDEAS;
            g_own[c][d] = (int8_t)n;
            if (n>0) g_any[c] = 1;
        }
    }
}

/* Le produit BRUT (non clampé) des idées effectives portant `key`. */
static float doct_raw_mult(int cid, const char *key){
    float m = 1.f;
    for (int d=0; d<DOCT_COUNT; d++){
        int n = g_own[cid][d];
        if (n<=0) continue;
        const DoctDef *dd = &DOCT_DEF[d];
        for (int i=0; i<n && i<DOCT_IDEAS; i++){
            const DoctIdeaDef *id = &dd->idea[i];
            if (!id->cable) continue;                    /* idée non câblée : aucun effet moteur */
            if ((id->cable & 1) && id->k1 && strcmp(id->k1, key)==0) m *= id->m1;
            if ((id->cable & 2) && id->k2 && strcmp(id->k2, key)==0) m *= id->m2;
        }
    }
    return m;
}

float doctrine_key_mult(int cid, const char *key){
    if (cid<0 || cid>=SCPS_MAX_COUNTRY || !key) return 1.f;
    if (!g_any[cid]) return 1.f;                          /* O(1) : la chronique et toute l'IA sortent ici */
    int n = g_cn[cid];
    for (int i=0;i<n;i++){
        const char *k = g_cache[cid][i].k;
        if (k==key || (k && strcmp(k,key)==0)) return g_cache[cid][i].m;
    }
    float m = doct_raw_mult(cid, key);
    if (m < 0.60f) m = 0.60f; else if (m > 1.60f) m = 1.60f;   /* clamp H2bis, PAR CLÉ */
    if (n < DOCT_CACHE){ g_cache[cid][n].k = key; g_cache[cid][n].m = m; g_cn[cid] = (uint8_t)(n+1); }
    return m;
}

/* ====================================================================== */
/* L'ÉTAT                                                                  */
/* ====================================================================== */
void doctrines_init(DoctrineState *ds){
    if (!ds) return;
    memset(ds, 0, sizeof *ds);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++)
        for (int s=0;s<DOCT_SLOTS_MAX;s++) ds->doct[c][s]=-1;
    doctrines_sync(ds);
}

static bool cid_ok(int cid){ return cid>=0 && cid<SCPS_MAX_COUNTRY; }

/* SIX slots, LIBRES dès la genèse (décision joueur 2026-09-02) — aucune
 * ouverture progressive : le frein est le COÛT, rien d'autre. */
int doctrines_slots_open(const DoctrineState *ds, int cid){
    if (!ds || !cid_ok(cid)) return 0;
    return DOCT_SLOTS_MAX;
}
int doctrines_at(const DoctrineState *ds, int cid, int slot){
    if (!ds || !cid_ok(cid) || slot<0 || slot>=DOCT_SLOTS_MAX) return -1;
    int d = ds->doct[cid][slot];
    return (d>=0 && d<DOCT_COUNT) ? d : -1;
}
int doctrines_slot_of(const DoctrineState *ds, int cid, int doctrine){
    if (!ds || !cid_ok(cid)) return -1;
    for (int s=0;s<DOCT_SLOTS_MAX;s++) if (ds->doct[cid][s]==doctrine) return s;
    return -1;
}
int doctrines_ideas_of(const DoctrineState *ds, int cid, int doctrine){
    if (!ds || !cid_ok(cid) || doctrine<0 || doctrine>=DOCT_COUNT) return 0;
    int n = ds->ideas[cid][doctrine];
    if (n<0) n=0;
    if (n>DOCT_IDEAS) n=DOCT_IDEAS;
    return n;
}
int doctrines_n_active(const DoctrineState *ds, int cid){
    if (!ds || !cid_ok(cid)) return 0;
    int n=0; for (int s=0;s<DOCT_SLOTS_MAX;s++) if (doctrines_at(ds,cid,s)>=0) n++;
    return n;
}
int doctrines_n_ideas(const DoctrineState *ds, int cid){
    if (!ds || !cid_ok(cid)) return 0;
    int n=0;
    /* Σ des idées des doctrines ENCORE TENUES : abandonner LIBÈRE le compte
     * (annexe §Coûts — « Abandonner libère le compte »). */
    for (int s=0;s<DOCT_SLOTS_MAX;s++){
        int d = doctrines_at(ds,cid,s);
        if (d>=0) n += doctrines_ideas_of(ds,cid,d);
    }
    return n;
}
/* L'ÉCHELLE D'ASSIETTE (influence_scale) LINÉARISE tous les prix sur la
 * population de l'assiette : deux fois plus de nobles = deux fois plus de gain
 * ET deux fois plus cher. Bornée ici aussi (défense en profondeur : l'appelant
 * a déjà appliqué le plancher 0.25 ; un `ech` absurde ne doit pas déborder). */
static float ech_ok(float ech){
    if (!(ech > 0.f)) return 1.f;      /* NaN/0/négatif : prix plats */
    if (ech > 1000.f) return 1000.f;
    return ech;
}
float doctrines_adopt_cost_f(const DoctrineState *ds, int cid, float ech){
    float base = tune_f("DOCT_COST_BASE", 50.f), step = tune_f("DOCT_COST_STEP", 25.f);
    return (base + step * (float)doctrines_n_active(ds,cid)) * ech_ok(ech);
}
float doctrines_idea_cost_f(const DoctrineState *ds, int cid, float ech){
    float base = tune_f("IDEA_COST_BASE", 30.f), step = tune_f("IDEA_COST_STEP", 3.f);
    return (base + step * (float)doctrines_n_ideas(ds,cid)) * ech_ok(ech);
}
int doctrines_adopt_cost(const DoctrineState *ds, int cid, float ech){
    return (int)(doctrines_adopt_cost_f(ds,cid,ech) + 0.5f);
}
int doctrines_idea_cost(const DoctrineState *ds, int cid, float ech){
    return (int)(doctrines_idea_cost_f(ds,cid,ech) + 0.5f);
}
int doctrines_current(const DoctrineState *ds, int cid){
    if (!ds || !cid_ok(cid)) return -1;
    for (int s=0;s<DOCT_SLOTS_MAX;s++){
        int d = doctrines_at(ds,cid,s);
        if (d>=DOCT_CURRENT_FIRST && d<DOCT_COUNT) return d;
    }
    return -1;
}

/* Les DEUX seules règles d'exclusivité (§4.1) — aucune autre. */
static bool doct_is_current(int d){ return d>=DOCT_CURRENT_FIRST && d<DOCT_COUNT; }
static int  doct_opposite(int d){
    if (d==DOCT_COMMERCE)       return DOCT_MERCANTILISME;
    if (d==DOCT_MERCANTILISME)  return DOCT_COMMERCE;
    return -1;
}

int doctrines_why_not(const DoctrineState *ds, const InfluenceState *is,
                      int cid, int slot, int doctrine, float ech){
    if (!ds || !cid_ok(cid) || doctrine<0 || doctrine>=DOCT_COUNT) return DOCT_NO_SLOT;
    if (doctrines_slot_of(ds,cid,doctrine)>=0) return DOCT_ALREADY;
    { int opp = doct_opposite(doctrine);
      if (opp>=0 && doctrines_slot_of(ds,cid,opp)>=0) return DOCT_EXCLUSIVE_PAIR; }
    if (doct_is_current(doctrine)){
        for (int s=0;s<DOCT_SLOTS_MAX;s++){
            int d = doctrines_at(ds,cid,s);
            if (d>=0 && doct_is_current(d)) return DOCT_EXCLUSIVE_CURRENT;
        }
    }
    if (slot>=0){
        if (slot>=DOCT_SLOTS_MAX || slot>=doctrines_slots_open(ds,cid)) return DOCT_NO_SLOT;
        if (doctrines_at(ds,cid,slot)>=0) return DOCT_NO_SLOT;
    } else {
        /* pas de slot demandé : y en a-t-il UN de libre ? (le catalogue s'en sert) */
        int free_slot=-1, open=doctrines_slots_open(ds,cid);
        for (int s=0;s<open && free_slot<0;s++) if (doctrines_at(ds,cid,s)<0) free_slot=s;
        if (free_slot<0) return DOCT_NO_SLOT;
    }
    if (is && !influence_can_spend(is, cid, doctrines_adopt_cost_f(ds,cid,ech)))
        return DOCT_NO_INFLUENCE;
    return DOCT_OK;
}

/* ====================================================================== */
/* LES VERBES                                                              */
/* ====================================================================== */
/* A6 (télémétrie chronicle, 2026-09-02) : le compteur « doctrines actives »
 * de chronicle.c est un INSTANTANÉ des pays vivants — il RECULE quand des pays
 * meurent, ce qui se lit à l'envers (« les doctrines disparaissent ») alors que
 * rien ne se désadopte (v107 : aucun entretien, une doctrine adoptée reste
 * ALLUMÉE). Ce compteur MONDE cumule les adoptions RÉUSSIES (joueur ET IA —
 * chronicle est headless, seule l'IA agit, donc en pratique il ne compte QUE
 * l'IA), jamais décrémenté, jamais sérialisé, RAZ explicite par sim (côté
 * chronicle.c, doctrines_adopt_total_reset). WRITE-ONLY ici : ce fichier ne
 * fait qu'incrémenter, aucun comportement de jeu n'en dépend. */
static long g_doct_adopt_total = 0;
long doctrines_adopt_total_get(void){ return g_doct_adopt_total; }
void doctrines_adopt_total_reset(void){ g_doct_adopt_total = 0; }

int doctrines_adopt(DoctrineState *ds, InfluenceState *is, int cid, int slot, int doctrine, float ech){
    if (!ds || !cid_ok(cid)) return 0;
    if (slot<0 || slot>=DOCT_SLOTS_MAX) return 0;
    if (doctrines_why_not(ds, is, cid, slot, doctrine, ech) != DOCT_OK) return 0;
    float cost = doctrines_adopt_cost_f(ds,cid,ech);
    if (is) influence_spend(is, cid, cost);
    ds->doct[cid][slot] = (int8_t)doctrine;
    ds->ideas[cid][doctrine] = 0;   /* on repart de zéro (abandon = idées perdues) */
    doctrines_sync(ds);
    g_doct_adopt_total++;   /* A6 — télémétrie chronicle uniquement, cf. commentaire ci-dessus */
    return 1;
}

int doctrines_buy_idea(DoctrineState *ds, InfluenceState *is, int cid, int doctrine, float ech){
    if (!ds || !cid_ok(cid) || doctrine<0 || doctrine>=DOCT_COUNT) return 0;
    int slot = doctrines_slot_of(ds, cid, doctrine);
    if (slot<0) return 0;                                   /* doctrine non adoptée */
    int n = doctrines_ideas_of(ds, cid, doctrine);
    if (n >= DOCT_IDEAS) return 0;                          /* complète : plus rien à acheter */
    float cost = doctrines_idea_cost_f(ds, cid, ech);
    if (is && !influence_can_spend(is, cid, cost)) return 0;
    if (is) influence_spend(is, cid, cost);
    ds->ideas[cid][doctrine] = (int8_t)(n+1);               /* SÉQUENTIEL : la prochaine, jamais un choix */
    doctrines_sync(ds);
    return 1;
}

int doctrines_abandon(DoctrineState *ds, InfluenceState *is, int cid, int slot){
    (void)is;   /* ABANDON LIBRE : aucun remboursement, aucune cicatrice (§4.1) */
    if (!ds || !cid_ok(cid) || slot<0 || slot>=DOCT_SLOTS_MAX) return 0;
    int d = doctrines_at(ds, cid, slot);
    if (d<0) return 0;
    ds->ideas[cid][d]  = 0;      /* les idées achetées sont PERDUES */
    ds->doct [cid][slot] = -1;
    doctrines_sync(ds);
    return 1;
}
