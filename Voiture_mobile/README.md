# Système V2V – Transfert d'énergie véhicule à véhicule

Contrôleur Arduino pour un système de transfert d'énergie bidirectionnel (Vehicle-to-Vehicle) gérant automatiquement les modes **Onduleur** et **Redresseur**, avec commande moteur Bluetooth, protection batterie, et affichage LCD.

---

## Matériel requis

| Composant | Détails |
|---|---|
| Microcontrôleur | Arduino Uno / Nano |
| Module Bluetooth | HC-05 ou HC-06 |
| Driver moteur | Pont en H (L298N ou équivalent) |
| Relais | 3× relais 5 V |
| Afficheur | LCD 16×2 avec interface I²C (adresse `0x27`) |
| Capteur de position | Fin de course NO (normalement ouvert) |
| Mesure de tension | Pont diviseur sur `A0` |

---

## Brochage (pinout)

| Fonction | Pin Arduino |
|---|---|
| Bluetooth RX | 5 |
| Bluetooth TX | 6 |
| Moteur IN1 | 7 |
| Moteur IN2 | 8 |
| PWM 20 kHz | 9 |
| Relais – Ondo/Bat/Redro | 2 |
| Relais – Ondo/Bob/Redro | 3 |
| Relais – Ground | 4 |
| Fin de course | 12 (INPUT\_PULLUP) |
| Tension primaire | A0 |
| LCD SDA / SCL | A4 / A5 |

---

## Fonctionnalités

### Sélection automatique du mode
Le système compare en permanence la tension primaire (`tension_propre`) à la tension secondaire (`tension_second`) et bascule automatiquement entre les deux modes :

- **Onduleur** : la batterie primaire alimente le secondaire.
- **Redresseur** : le secondaire recharge la batterie primaire.

Une hystérésis de ±0,5 V (`HYSTERESIS`) évite les basculements répétés autour du seuil.

### Protection batterie faible
Si la tension primaire descend sous `VMIN` (par défaut **6,0 V**) :

- Le mode Onduleur est révoqué immédiatement.
- Le système passe en Redresseur.
- Le LCD affiche `BAT FAIBLE` avec la tension mesurée.
- Une notification `BAT FAIBLE` est envoyée via Bluetooth.

La protection se lève uniquement lorsque la tension remonte au-dessus de `VMIN + VMIN_HYST` (6,2 V), grâce à l'hystérésis qui évite les oscillations.

### Commande moteur Bluetooth
Le moteur est commandé à distance via Bluetooth avec les caractères suivants :

| Commande | Action |
|---|---|
| `A` | Sens 1 |
| `B` | Sens 2 |
| `X` | Arrêt système |
| `S` | Activation système |

### Gestion du fin de course
Le système utilise un **unique fin de course** pour les deux butées mécaniques :

- Quand le fin de course se déclenche, le **sens fautif est mémorisé et bloqué**.
- Le **sens opposé reste autorisé** immédiatement, même si le capteur est encore physiquement actif.
- Une fenêtre de déblocage de `TEMPS_DEBLOCAGE` (500 ms) laisse le temps au mécanisme de quitter la butée avant de rétablir la surveillance normale du capteur.

### Affichage LCD
L'écran 16×2 affiche en permanence l'état du système.

**Mode normal :**
```
ONDULEUR
P:12.4 S:11.8
```

**Batterie faible :**
```
BAT FAIBLE
P=5.8V
```

**Système arrêté :**
```
SYSTEME ARRETE
```

Le rafraîchissement est limité à une fois toutes les **500 ms** et l'écran n'est réécrit complètement que si l'état change, afin d'éviter les scintillements.

---

## Architecture logicielle

### Boucle non-bloquante
L'ensemble du code est conçu sans aucun `delay()` dans la boucle principale. Ceci est essentiel car `SoftwareSerial` perd les octets entrants pendant les appels bloquants.

Les temporisations sont toutes gérées avec `millis()` :

| Temporisation | Durée | Rôle |
|---|---|---|
| `DELAI_RELAIS` | 100 ms | Stabilisation des relais avant activation PWM |
| `TEMPS_DEBLOCAGE` | 500 ms | Sortie de butée moteur |
| `PERIODE_LCD` | 500 ms | Rafraîchissement écran |

### Machine d'état relais
Le changement de mode (Onduleur ↔ Redresseur) passe par une machine d'état à deux étapes :
1. Configuration immédiate des relais + coupure PWM.
2. Activation du PWM après `DELAI_RELAIS` ms, sans bloquer la boucle.

---

## Paramètres configurables

Tous les seuils sont regroupés en tête de fichier pour faciliter l'adaptation :

```cpp
const float HYSTERESIS   = 0.5;   // Hystérésis changement de mode (V)
const float VMIN         = 6.0;   // Seuil batterie faible (V)
const float VMIN_HYST    = 0.2;   // Hystérésis protection batterie (V)
const float facteur      = 4.4531835; // Facteur diviseur de tension A0

const unsigned long TEMPS_DEBLOCAGE = 500; // Délai sortie de butée (ms)
const unsigned long DELAI_RELAIS    = 100; // Délai stabilisation relais (ms)
const unsigned long PERIODE_LCD     = 500; // Rafraîchissement LCD (ms)
```

---

## Entrée de la tension secondaire

La tension secondaire (`tension_second`) est lue **une seule fois** au démarrage via le port série (`Serial`) :

```
# Depuis le moniteur série Arduino IDE
> 11.8
```

La valeur doit être un flottant entre 0 et 100. Une fois reçue, la variable `tensionRecue` passe à `true` et le choix automatique de mode s'active.

---

## Librairies requises

À installer depuis le gestionnaire de librairies Arduino IDE :

- `LiquidCrystal_I2C` — Frank de Brabander
- `Wire` — incluse dans Arduino IDE
- `SoftwareSerial` — incluse dans Arduino IDE

---

## Licence

MIT — libre d'utilisation, de modification et de distribution.
