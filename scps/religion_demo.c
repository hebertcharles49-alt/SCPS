/*
 * religion_demo.c — banc du module fondation religion (P1).
 * Appelle religion_selftest() : assert() abandonne (rc≠0) si un invariant casse —
 * axes (pole>>1==axe), un-pôle-par-axe, spawn/apply (somme pôles+crédo), schisme
 * (slot conservé + variante couleur proche). Module PUR : aucun moteur lié.
 */
#include "scps_religion.h"
#include <stdio.h>

int main(void){
    printf("== religion : module fondation (16 pôles · 3 crédos) ==\n");
    religion_selftest();
    printf("   \xe2\x9c\x93 selftest OK : axes · un-par-axe · spawn/apply · schisme · couleur\n");
    printf("\n== BILAN : religion OK ==\n");
    return 0;
}
