# City mission scripts: Madrid, Tokyo, Rome (for review)

These scripts are already **built and playable** (you chose "build first, review later"). Edit anything here, or change
the source directly in `Tools/Content/{madrid,tokyo,rome}_content.py` (or in the Conversation Editor), and Claude will
regenerate. Accepted answers ignore case and punctuation (including ¿¡ and 。、), but accents count.

## Madrid (Spanish, Spain): "La Tapa Secreta"
Flow: Javier (tapas bar) → Carmen (plaza, directions) → walk to the metro entrance → Diego (code phrase).
Blowing your cover with Diego fails the mission (restart).

| NPC | Line | Helper | Player says | Skill |
|---|---|---|---|---|
| Javier | ¡Hola! ¿Qué le pongo? | Hi! What can I get you? | *Hola, una tapa de tortilla, por favor* / *Una tapa de tortilla, por favor* | Ordering |
| Javier | ¿Algo para beber? | Anything to drink? | **CHOICE:** *Un agua, por favor* · *Un café con leche, por favor* | Ordering |
| Javier | Son cuatro euros. | That's four euros. | *Aquí tiene cuatro euros* | Numbers |
| Javier | ¡Gracias! Por cierto... alguien le espera cerca del metro. (END) | …someone is waiting for you near the metro. | | |
| Carmen | ¿Necesita ayuda? | Do you need help? | *Sí, ¿dónde está el metro?* / *¿Dónde está el metro?* | Directions |
| Carmen | Siga todo recto y luego gire a la derecha. | Straight ahead, then turn right. | | |
| Carmen | ¿Está claro? | Is that clear? | *Muchas gracias* | Politeness |
| Diego | Hace calor hoy, ¿verdad? | It's hot today, isn't it? *(code)* | *Sí, pero la noche será fría* | Code |
| Diego | Bien. ¿Tiene la llave? | Good. Do you have the key? | *Sí, aquí está* | Code |
| Diego | Perfecto. La carpeta está en el palacio. ¡Suerte! (END) | The folder is at the palace. Good luck! | | |

## Tokyo (Japanese): "Konbini Contact"
Flow: Sato (konbini) → Tanaka (street, directions) → walk to the shrine torii → Yuki (code phrase).
Tiles are word chunks with romaji; the "Romaji" button toggles readings.

| NPC | Line (romaji) | Helper | Player says (tiles) | Skill |
|---|---|---|---|---|
| Sato | いらっしゃいませ！ (Irasshaimase!) | Welcome! | おにぎり・を・ください (onigiri o kudasai) | Ordering |
| Sato | あたためますか？ (Atatamemasu ka?) | Shall I warm it up? | **CHOICE:** はい・おねがいします · いいえ・けっこうです | Politeness |
| Sato | ひゃくごじゅう円です。 (Hyaku gojuu en desu.) | That's 150 yen. | はい・どうぞ / どうぞ | Shopping |
| Sato | ありがとうございました！ (END) | Thank you very much! | | |
| Tanaka | どうしましたか？ (Dou shimashita ka?) | What's the matter? | すみません・駅・は・どこ・ですか / 駅・は・どこ・ですか | Directions |
| Tanaka | まっすぐ いって、みぎです。 | Go straight, then it's on the right. | | |
| Tanaka | わかりましたか？ | Did you understand? | ありがとう・ございます | Politeness |
| Yuki | さくらが きれいですね。 | The cherry blossoms are pretty… *(code)* | はい・でも・すぐ・ちります (hai, demo sugu chirimasu) | Code |
| Yuki | では、これを どうぞ。 | Then, please take this. | ありがとう・ございます | Politeness |
| Yuki | きをつけて。 (END) | Be careful. | | |

## Rome (Italian): "Protocollo Espresso"
Flow: Luca (bar) → Giulia (piazza, directions) → walk to the fountain → Marco (code phrase).

| NPC | Line | Helper | Player says | Skill |
|---|---|---|---|---|
| Luca | Buongiorno! Cosa prende? | Good morning! What will you have? | *Buongiorno, un caffè, per favore* / *Un caffè, per favore* | Ordering |
| Luca | Al banco o al tavolo? | Counter or table? | **CHOICE:** *Al banco, grazie* (local style) · *Al tavolo, per favore* | Ordering |
| Luca | Sono un euro e venti. | That's one euro twenty. | *Ecco a lei* | Shopping |
| Luca | Grazie! Ah... un signore la cerca vicino alla fontana. (END) | …a gentleman is looking for you near the fountain. | | |
| Giulia | Posso aiutarla? | Can I help you? | *Scusi, dov'è la fontana?* / *Dov'è la fontana?* | Directions |
| Giulia | Sempre dritto, poi a sinistra. | Straight on, then left. | | |
| Giulia | Chiaro? | Clear? | *Grazie mille* | Politeness |
| Marco | Bella giornata, vero? | Lovely day, isn't it? *(code)* | *Sì, ma domani pioverà* | Code |
| Marco | Ha una moneta per la fontana? | Do you have a coin for the fountain? | *Sì, eccola* | Code |
| Marco | Perfetto. I documenti sono al Colosseo. Buona fortuna. (END) | The documents are at the Colosseum. | | |
