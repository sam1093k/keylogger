# KeyLogger

Registratore di tasti personale per Windows, scritto in C++ con le sole API native (nessuna libreria esterna).

Ad ogni avvio scrive/aggiorna un file di testo in `Documenti\KeyLog\keylog_AAAA-MM-GG.txt`, ricostruendo il testo digitato (lettere, numeri, accenti, simboli AltGr, Invio, Tab, Backspace).

## Compilazione

```
g++ -O2 -std=c++17 keylog.cpp -o keylog.exe -luser32
```

## Uso

Avvia `keylog.exe`: rimane una finestra di console visibile finché non la chiudi o premi CTRL+C.

Pensato per uso personale sul proprio computer: non ha modalità nascoste, non si avvia automaticamente e non invia dati altrove, tutto resta nel file di testo locale. Non va usato su computer di altre persone senza il loro consenso.
