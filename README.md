# rozproszone-pszczolki

Oto tekst w takiej postaci, aby można go było wkleić do pliku README:

---

# Pszczółki murarki

Małe, wesołe pszczółki murarki budzą się wczesną wiosną i zaczynają zbierać nektar. Jest P pszczółek, które konkurują w dostępie najpierw, na samym początku działania, do jednej z T rozróżnialnych trzcin, a potem do szklarni, w której jest K nierozróżnialnych kwiatków. Po zebraniu nektaru pszczółka składa jajo w kokonie. Po złożeniu 5 jaj pszczółka umiera i trzcinę może zająć inna pszczółka. Maksymalnie w trzcinie zmieści się 15 kokonów. Początkowe priorytety pszczółek (np. zegary lamporta) powinny być losowe.

## Struktury i zmienne

- **B** - liczba pszczółek
- **R** - liczba trzcin
- **F** - liczba kwiatków
- **egg_count** - liczba złożonych jaj przez daną pszczółkę
- **cocoon_count** - liczba zajętych kokonów w danej trzcinie
- **flower_que** - kolejka pszczółek ubiegających się o kwiatek
- **reed_que** - kolejka pszczółek ubiegających się o trzcinę

## Wiadomości

Wszystkie wiadomości są oznaczone znacznikiem czasowym (timestampem), modyfikowanym zgodnie z zasadami skalarnego zegara logicznego Lamporta.

- **REED_REQ [nr] [rid] [req]** - żądanie dostępu do n-tej trzciny
- **FLW_REQ [nr] [rid] [fid] [req]** - żądanie dostępu do n-tego kwiatka
- **REED_ACK [rid] [req]** - potwierdzenie dostępu do sekcji krytycznej z trzciną
- **FLW_ACK [fid] [req]** - potwierdzenie dostępu do sekcji krytycznej z kwiatkiem
- **ENTER_REED [nr] [rid]** - wejście na trzcinę
- **ENTER_FLW [nr] [fid]** - wejście na kwiatek
- **COCOON [nr] [rid]** - złożenie jajka w kokonie przez pszczółkę
- **END_FLW [nr] [rid] [fid] [req]** - koniec składania nektaru, powrót do trzciny
- **DEAD [nr]** - komunikat o śmierci pszczółki

  - **rid** - numer identyfikujący trzcinę
  - **fid** - numer identyfikujący kwiatek
  - **nr** - numer procesu
  - **req** - lokalny indeks żądania i odpowiedzi

## STANY

- **REST**: Początkowy stan, pszczółki nie ubiegają się o dostęp do trzciny lub szklarni.
- **WAIT_REED**: Stan oczekiwania na możliwość dostępu do trzciny.
- **ON_REED**: Stan zajmowania trzciny i składania jaj.
- **WAIT_FLOWER**: Stan oczekiwania na możliwość dostępu do kwiatka.
- **ON_FLOWER**: Stan zajmowania kwiatka.

## Algorytm

### Dostęp do trzciny:

1. Pszczółka rozsyła **REED_REQ** do wszystkich innych pszczółek i ustawia licznik zgód na 0.
2. Po otrzymaniu **REED_REQ** od innej pszczółki:
   - Jeśli pszczółka nie ubiega się o sekcję, wysyła **REED_ACK**.
   - Jeśli pszczółka ma wyższy priorytet, nie odsyła, ale zapamiętuje w kolejce.
   - Nie odsyła **REED_ACK**, jeśli wszystkie trzciny są zajęte.
3. Pszczółka, jeśli otrzyma zgody od wszystkich innych pszczółek, zajmuje trzcinę i rozsyła wiadomość **ENTER_REED**.
4. Po śmierci (DEAD) modyfikuje liczbę pszczółek B (-1).

### Dostęp do szklarni:

1. Wysyła do wszystkich pszczółek **FLW_REQ** i zeruje licznik ACK.
2. Po otrzymaniu **FLW_REQ** od innej pszczółki:
   - Jeśli pszczółka nie ubiega się, wysyła **FLW_ACK**.
   - Jeśli ubiega się, ale ma niższy priorytet, wysyła **FLW_ACK**. Jak ma wyższy, to nie wysyła **FLW_ACK** i zapamiętuje w kolejce.
3. Jeśli pszczółka otrzymała **B-(F-flw_occupied) ACK** - wchodzi do szklarni.
   - Licznik Lamporta nie może być mniejszy od licznika wiadomości **FLW_REQ**.
4. Po wejściu do szklarni zwiększa licznik **flw_occupied** i rozsyła **ENTER_FLW**.
5. Po wyjściu ze szklarni wysyła **END_FLW** i wszystkie pszczółki zmniejszają **flw_occupied**.

### Składanie jaj i koniec życia pszczółki:

1. Po wysłaniu wiadomości **END_FLW**, pszczółka wraca na trzcinę, składa jajo i wysyła wiadomość **COCOON** z numerem trzciny (rid) i numerem procesu (nr).
2. Jeśli złożyła mniej niż 5 jajek, pszczółka wchodzi w stan **WAIT_FLOWER**.
3. Po złożeniu maksymalnej liczby jaj (5), pszczółka umiera, a trzcina staje się ponownie dostępna dla innych pszczółek, jeśli jest na niej mniej niż 15 kokonów.

Algorytm wykonuje się do zakończenia życia ostatniej pszczółki.
