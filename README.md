# Omezení projektu

_Žádné známé chyby či omezení aplikace._


# Práce s programem
## Spuštění

Použití socketů typu `SOCK_RAW` v implementaci vyžaduje při spuštění oprávnění **superusera**.

```bash
sudo ./ipk-dhcpstarve -i <interface>
```

Program má jeden povinný přepínač obsahující název síťového interfacu počítače, ze kterého bude odesílat kompromitační data do sítě.


## Výstup

Po přijetí `DHCPACK` od DHCP serveru je IP adresa, které se program zmocnil vypsána na `stdout`. Každá získaná adresa je vypsána na nový řádek.

Program se sám ukončí při vyčerpání pokusů o opětovné odeslání `DHCPREQUEST` a neobdržení žádného `DHCPOFFER` packetu během specifikovaného časového intervalu (více informací v sekci [definované konstanty](#definovane-konstanty)).


## Překlad

Příkazem `make` či `make all` dojde ke kompletnímu přeložení projektu bez ladících výpisů.

Je možné použít i příkaz `make debug`, v takovém případě jsou v programu povoleny všechny typy ladících výpisů. Všechny ladící výpisy jsou zapisovány na `stderr`.


## Definované konstanty

Všechny níže uvedené konstanty je možné při překladu předefinovat a změnit tak jejich výchozí hodnoty.

| Název konstanty      | Výchozí hodnota | Popis konstanty |
|----------------------|-----------------|-----------------|
| `BUFFER_SIZE`        | `1500`          | Maximální velikost bufferů pro odesílání a příjem.
| `SOCKET_TIMEOUT`     | `4`             | _Ve vteřinách._ Určuje maximální dobu čekání při `sendto` a `recvfrom`.
| `SOCKET_RETRY_COUNT` | `4` | Určuje maximální počet pokusů o opětovné odeslání předcházejících packetů. (při neobdržení `DHCPOFFER` se jedná o `DHCPDISCOVER`, při neobdržení `DHCPACK` o `DHCPREQUEST`)
| `OPERATION_TIMEOUT`  | `(SOCKET_TIMEOUT * SOCKET_RETRY_COUNT) * 1000` | _V milisekundách._ Určuje maximální dobu provádění operace. (př.: čekání na `DHCPOFFER` a opětovného odesílání `DHCPDISCOVER`) Rozhoduje o "úspěšném ukončení" programu.
