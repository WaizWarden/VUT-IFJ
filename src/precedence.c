//Hlavní smyčka - prezentace 8
/* push $ na zásobník
   smyčka:
     a = nejvyšší TERMINÁL na zásobníku (přeskoč neterminály!)
     b = aktuální token ze scanneru
     switch TABLE[a][b]:
       '='  → push(b); načti další token
       '<'  → za a vlož značku '<'; push(b); načti další token
       '>'  → najdi handle: od vrcholu po značku '<' = pravá strana y;
              porovnej s pravidly, proveď redukci (viz krok 3);
              žádné pravidlo nesedí → exit(2)
       prázdno → exit(2)
   konec: b == $ a zásobník = [$ E]
   */
