# Predlog projekat: Bežični brojač poena za stoni tenis

**Predmet:** Merni sistemi zasnovani na mikroračunarima
**Autor:** Dušan Dimitrijević

---

## 1. Opis i svrha projekta

Cilj projekta je izrada bežičnog elektronskog sistema za praćenje rezultata u stonom tenisu, bez potrebe za sudijom ili neutralnom osobom. Svaki igrač ima sopstvene dugmiće kojima samostalno beleži poene, čime se eliminiše potreba za ručnim praćenjem rezultata ili verbalnom komunikacijom između igrača čime se stvara mogućnost za grešku u praćenju rezultata.

Sistem je posebno koristan u neformalnim uslovima igranja kao što su školske sale, rekreativni centri i kod kuće, gde sudija nije prisutan. Rezultat se prikazuje na ekranu koji je vidljiv obojici igrača u realnom vremenu.

---

## 2. Zahtevi projekta

### Funkcionalni zahtevi

- Svaki igrač ima **dva dugmeta**: jedno za dodavanje poena i jedno za oduzimanje (korekcija greške)
- Pritisak na dugme bežično šalje informaciju o promeni rezultata na prijemnu jedinicu
- Prijemna jedinica prikazuje trenutni rezultat na ekranu
- Sistem automatski prepoznaje kraj seta (prvi igrač koji osvoji 11 poena sa razlikom od najmanje 2)
- Nakon osvajanja seta, rezultat se resetuje na 0:0, a broj setova se uvećava
- **Rotacija strana**: nakon svakog seta igrači menjaju strane stola. Sistem automatski zamenjuje mapiranje dugmadi (dugmad igrača A postaju dugmad igrača B i obrnuto), tako da svaki igrač uvek pritiska svoja fizička dugmad bez obzira na stranu na kojoj se nalazi. Ekran prikazuje indikator koja strana pripada kom igraču.
- **Zvučna signalizacija** putem buzzera na prijemnoj jedinici razlikuje sve događaje u igri:
  - Kratak visok ton - registrovan poen
  - Niži duži ton - oduzimanje poena (korekcija greške)
  - Uzlazna trotaktna melodija - osvajanje seta
  - Srednji ton - reset ili nova igra
  - Startup signal pri uključivanju uređaja
  - **Merenje dužine razmene**: senzor vibracija SW-420 montiran na predajnoj jedinici (sto) detektuje svaki odbitak loptice. Broj odbitaka se šalje prijemniku kao posebna poruka i prikazuje se na ekranu kao statistika trenutnog poena. Na kraju meča prikazuje se i najduža razmena.
- **Kombinovani pritisak** dugmadi oba igrača istovremeno u trajanju od 2 sekunde pokreće posebne komande:
  - Oba igrača drže **+** dugme (A+ i B+): početak novog meča resetuje rezultat i setove
  - Oba igrača drže **−** dugme (A− i B−): resetovanje trenutnog seta

### Nefunkcionalni zahtevi

- Sistem mora raditi **bez Wi-Fi rutera**, direktna bežična komunikacija između uređaja
- Kašnjenje od pritiska dugmeta do promene na ekranu mora biti ispod 100 ms
- Sistem mora biti stabilan i bez neplaniranih restarta
- Kod mora biti čitljiv i dokumentovan, pogodan za dalji razvoj

---

## 3. Korišćene komponente

| Komponenta | Uloga u sistemu |
|---|---|
| Lolin NodeMCU v3 (ESP8266) | Predajnik - čita dugmiće i šalje podatke |
| WeMos D1 Mini (ESP8266) | Prijemnik - prima podatke i upravlja ekranom |
| OLED ekran 0.96" SSD1306 (SPI) | Prikaz rezultata |
| 4× taktilni prekidač (4-pina) | Unos poena od strane igrača |
| SW-420 senzor vibracija | Detekcija odbitaka loptice od stola |
| Pasivni buzzer | Zvučna signalizacija događaja u igri |
| Breadboard i kratkospojnici | Prototipska veza komponenti |

### Protokol komunikacije: ESP-NOW

Za bežičnu komunikaciju korišćen je **ESP-NOW protokol** - Espressifov vlasnički protokol za direktnu komunikaciju između ESP8266/ESP32 uređaja. Odabran je iz sledećih razloga:

- Ne zahteva Wi-Fi ruter niti pristupnu tačku
- Kašnjenje je ispod 1 ms
- Implementacija je jednostavna - ugrađen u ESP8266 Arduino core biblioteku
- Domet u otvorenom prostoru iznosi do 200 metara

---

## 4. Opis kompleksnosti

### Hardverska kompleksnost

Sistem se sastoji od dve fizički odvojene jedinice koje komuniciraju bežično:

- **Predajna jedinica** (NodeMCU): četiri dugmeta spojena na digitalne ulaze sa internim pull-up otpornicima. Bez eksternih otpornika. Dugmad su smeštena na breadboard-u i razdvojena po osama igrača. SW-420 senzor vibracija spojen na digitalni pin D7, detektuje mehanički impuls odbitka loptice od površine stola i šalje `RALLY` poruku prijemniku.
- **Prijemna jedinica** (D1 Mini): SPI OLED ekran spojen na 6 pinova (SCL, SDA, DC, RES, GND, VCC). Ekran osvežava sliku svaki put kada se primi nova poruka. Pasivni buzzer spojen direktno na pin D0 i GND - bez eksternih otpornika.

### Softverska kompleksnost

- **Debouncing dugmadi** bez `delay()` funkcije - korišćenjem praćenja stanja i vremenskih oznaka
- **Detekcija kombinovanog pritiska** dugmadi oba igrača istovremeno sa vremenskim pragom od 2 sekunde A+ i B+ zajedno pokreću novi meč, A− i B− zajedno resetuju set. Ovaj dizajn zahteva saglasnost oba igrača za svaku specijalnu komandu, čime se sprečava slučajno aktiviranje.
- **ESP-NOW callback rukovanje**: prijem podataka odvija se u prekidnoj rutini koja mora biti kratka. Svi sporedni procesi (ažuriranje ekrana, logika rezultata) prebačeni su u `loop()` funkciju korišćenjem volatile bita (`volatile bool`), čime se sprečava pad sistema usled prekoračenja watchdog tajmera
- **Automatsko praćenje setova** prema pravilima stonog tenisa (11 poena, razlika 2)
- **Rotacija strana između setova**: logički volatile bit (`sidesSwapped`) prati da li su strane zamenjene. Prijem poruka ostaje nepromenjen, remapiranje A↔B akcija vrši se u `loop()` funkciji pre obrade rezultata, čime se postiže transparentna zamena bez ikakvih izmena na predajniku. Komanda `NEWGAME` resetuje flag na početnu vrednost.
- **Zvučna signalizacija**: četiri odvojene funkcije (`beepPoint()`, `beepUndo()`, `beepSet()`, `beepReset()`) generišu tonove različite frekvencije i trajanja korišćenjem `tone()` funkcije. Svaki događaj ima prepoznatljiv zvuk čime igrači dobijaju povratnu informaciju bez gledanja u ekran.
- **Statistika razmene**: prijemnik broji `RALLY` poruke po poenu, čuva vrednost najduže razmene i resetuje brojač pri svakom novom poenu.
- Kod je organizovan u odvojene funkcije: `onReceive()`, `updateDisplay()`, `checkSet()`, `beepPoint()`, `beepUndo()`, `beepSet()`, `beepReset()`

---

## 5. Šema povezivanja

### Predajnik - NodeMCU v3

```
NodeMCU v3
┌─────────────────────┐
│                     │
│  D1 (GPIO5)  ───────┼──── Dugme A+  ──── GND
│  D2 (GPIO4)  ───────┼──── Dugme A-  ──── GND
│  D5 (GPIO14) ───────┼──── Dugme B+  ──── GND
│  D6 (GPIO12) ───────┼──── Dugme B-  ──── GND
│  D7 (GPIO13) ───────┼──── SW-420 DO
│                     │
│  3V3, GND           │     (napajanje)
└─────────────────────┘

* Svi pinovi konfigurisani kao INPUT_PULLUP
* Dugme zatvara strujno kolo prema GND pritiskom
* SW-420: digitalni izlaz HIGH pri detekciji vibracije
```

### Prijemnik - WeMos D1 Mini + OLED + Buzzer

```
D1 Mini              OLED SSD1306 (SPI)
┌──────────┐         ┌──────────────┐
│          │         │              │
│  D5 ─────┼─────────┼─ SCL         │
│  D7 ─────┼─────────┼─ SDA (MOSI)  │
│  D1 ─────┼─────────┼─ DC          │
│  D3 ─────┼─────────┼─ RES         │
│  3V3 ────┼─────────┼─ VCC         │
│  GND ────┼─────────┼─ GND         │
│          │         └──────────────┘
│  D0 ─────┼──── Buzzer (+)
│  GND ────┼──── Buzzer (-)
│          │
└──────────┘

* Pasivni buzzer, bez eksternih otpornika
```

### Blok dijagram sistema

```
┌─────────────────────────────────────────────────────┐
│                  PREDAJNA JEDINICA                  │
│                                                     │
│  [Dugme A+]──┐                                      │
│  [Dugme A-]──┤                                      │
│              ├──► NodeMCU v3 ──► ESP-NOW ········►  │
│  [Dugme B+]──┤                                      │
│  [Dugme B-]──┘                                      │
└─────────────────────────────────────────────────────┘
                                          ·
                                          · (bežično)
                                          ·
┌─────────────────────────────────────────────────────┐
│                  PRIJEMNA JEDINICA                  │
│                                                     │
│  ············► D1 Mini ──► OLED ekran + Buzzer          │
│                                                     │
│                        ┌──────────────┐             │
│                        │  Setovi 1:0  │             │
│                        │ B<---  --->A │  (rotacija) │
│                        │   07: 03     │             │
│                        │ Rally: 12    │             │
│                        └──────────────┘             │
└─────────────────────────────────────────────────────┘
```

---

## 6. Korišćene biblioteke i razvojno okruženje

| Biblioteka / Alat | Namena |
|---|---|
| Arduino IDE | Razvojno okruženje |
| ESP8266 Arduino Core | Podrška za NodeMCU i D1 Mini |
| `espnow.h` | ESP-NOW bežična komunikacija |
| `ESP8266WiFi.h` | Inicijalizacija WiFi steka |
| `Adafruit SSD1306` | Upravljanje OLED ekranom |
| `Adafruit GFX` | Grafička biblioteka za tekst i oblike |
| `SPI.h` | SPI komunikacija sa ekranom |
| `tone()` (ugrađena) | Generisanje tonova za pasivni buzzer |

---

## 7. Zaključak

Projekat demonstrira praktičnu primenu bežične komunikacije između mikrokontrolera u realnom scenariju. Sistem je funkcionalan, pouzdan i proširiv. Sve komponente su niskonaponske (3.3V/5V) i bezbedne za rad, a ceo sistem može biti napajan powerbankom tokom igre.
