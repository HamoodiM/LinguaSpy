"""Paris / French content: conversations, the 'First Contact' tutorial mission and the language pack.
Run inside the editor:  exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/paris_content.py", encoding="utf-8").read())

Phase 5 retrofit (docs/Learning_Design.md, docs/Curriculum.md): briefing phrases, can-do statements, feedback hints,
intel checks, field notes, tutorial-level cover penalties. NodeIds are unchanged (SRS keys depend on them).
Run voice_build.py afterwards to (re)attach the spoken lines.
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

T, O = lc.T, lc.O
H = lc.hint
DATA = "/Game/LinguaSpy/Data/French"
MISSIONS = "/Game/LinguaSpy/Data/French/Missions"
P = 12.0      # tutorial: low stakes while the player is new (learning design section 9)
P_CODE = 20.0

paris = lc.language_pack("/Game/LinguaSpy/Data/French", "DA_Lang_French_Paris", "Paris", "French", "Paris", "fr",
                         "Croissants, cafés and coded whispers.", "8FB8DE", 0,
                         city_map="/Game/LinguaSpy/Maps/Lvl_Paris")

# --- Café: order a coffee, small talk choice ------------------------------------------------------------------
cafe = lc.conversation(DATA, "DA_Conv_Cafe", "Café terrace - order a coffee", paris, "c_greet", [
    lc.node("c_greet", T.PLAYER_CHALLENGE, "Bonjour ! Qu'est-ce que je vous sers ?", "Hello! What can I get you?", "c_small_talk",
            challenge_=lc.challenge("Greet her and politely order a coffee.",
                                    "Bonjour je voudrais un café s'il vous plaît veux thé merci",
                                    ["Bonjour je voudrais un café s'il vous plaît", "Bonjour un café s'il vous plaît"], "Ordering", P,
                                    gaps=[2, 4],
                                    hints=[H("Start with a greeting: Bonjour.", 0),
                                           H("Polite order: 'je voudrais' (I would like). 'Je veux' (I want) sounds rude to a stranger.", 2),
                                           H("Coffee is 'café' - tea would be 'thé'.", 4),
                                           H("Greeting, then what you'd like, then 'please': Bonjour... je voudrais... s'il vous plaît.")])),
    lc.node("c_small_talk", T.PLAYER_CHOICE, "Vous êtes touriste ?", "Are you a tourist?", "",
            choices=[
                lc.choice("Play the tourist: say you're on holiday.",
                          lc.challenge("Say: yes, I'm on holiday.", "Oui je suis en vacances travail non",
                                       ["Oui je suis en vacances"], "SmallTalk", P,
                                       hints=[H("'Yes' is 'oui'.", 0), H("'I am' = 'je suis'.", 2),
                                              H("'On holiday' = 'en vacances'.")]),
                          "c_size", flags=["CoverTourist"]),
                lc.choice("Stay vague: say you're here for work.",
                          lc.challenge("Say: no, I'm here for work.", "Non je suis ici pour le travail vacances oui",
                                       ["Non je suis ici pour le travail"], "SmallTalk", P,
                                       hints=[H("'No' is 'non'.", 0), H("'Here' = 'ici', 'for work' = 'pour le travail'.")]),
                          "c_size", flags=["CoverWork"]),
            ]),
    lc.node("c_size", T.PLAYER_CHALLENGE, "Très bien. Un grand ou un petit ?", "Very good. A large or a small?", "c_price",
            challenge_=lc.challenge("Ask for a small one, please.", "un petit s'il vous plaît grand deux",
                                    ["un petit s'il vous plaît", "petit s'il vous plaît"], "Ordering", P,
                                    hints=[H("Small = 'petit', large = 'grand'.", 1),
                                           H("'A small one, please': un petit, s'il vous plaît.")])),
    # Listening check (was an NPC line): understand the price before paying
    lc.intel("c_price", "Ça fait deux euros cinquante.", "That'll be two euros fifty.", "c_thanks",
             "How much does she want?", ["€2.50", "€12.50", "€2.15"], 0, skill="Numbers", cost=5.0),
    lc.node("c_thanks", T.PLAYER_CHALLENGE, "Voilà ! Bonne journée !", "Here you go! Have a good day!", "c_end",
            challenge_=lc.challenge("Thank her and wish her a good day too.",
                                    "Merci vous aussi bonne journée toi bonjour à",
                                    ["Merci vous aussi", "Merci bonne journée", "Merci à vous aussi"], "Politeness", P,
                                    hints=[H("Start with 'thank you': merci.", 0),
                                           H("'You too' to a stranger is 'vous aussi' (not 'toi').")])),
    lc.node("c_end", T.END, "Au revoir, monsieur !", "Goodbye, sir! (Nicely done - she didn't suspect a thing.)"),
])

# --- Kiosk: code phrase. Blowing your cover here makes Marcel suspicious (mission branches) ---------------------
kiosk = lc.conversation(DATA, "DA_Conv_Kiosk", "Newspaper kiosk - meet the contact", paris, "v_greet", [
    lc.node("v_greet", T.PLAYER_CHALLENGE, "Bonjour ! Vous cherchez quelque chose ?", "Hello! Are you looking for something?", "v_dir",
            challenge_=lc.challenge("Say hello and ask where the station is.",
                                    "Bonjour où est la gare s'il vous plaît le musée qui",
                                    ["Bonjour où est la gare", "Bonjour où est la gare s'il vous plaît",
                                     "Bonjour s'il vous plaît où est la gare"], "Directions", P,
                                    gaps=[1, 4],
                                    hints=[H("'Where is...?' = 'Où est...?'", 1), H("The station is 'la gare'.", 4),
                                           H("Hello, then 'where is the station': Bonjour, où est la gare ?")])),
    # Listening check (was an NPC line): the player repeats it back next, so they must have understood it
    lc.intel("v_dir", "La gare ? Allez tout droit, puis tournez à gauche.", "The station? Go straight ahead, then turn left.",
             "v_confirm", "Which way is the station?",
             ["Straight on, then right", "Straight on, then left", "Left, then straight on"], 1, skill="Directions", cost=5.0),
    lc.node("v_confirm", T.PLAYER_CHALLENGE, "C'est clair ?", "Is that clear?", "v_code",
            challenge_=lc.challenge("Repeat it back: straight ahead, then left.", "tout droit puis à gauche droite après",
                                    ["tout droit puis à gauche"], "Directions", P,
                                    hints=[H("'Straight ahead' = 'tout droit' (two words).", 0),
                                           H("Left is 'gauche'; 'droite' means right.", 4),
                                           H("Straight ahead, then (puis) to the left (à gauche).")])),
    lc.node("v_code", T.PLAYER_CHALLENGE, "Vous aimez les journaux d'aujourd'hui ?",
            "Do you like today's newspapers? (That's the code phrase! Reply: 'I prefer yesterday's newspaper.')", "v_end",
            challenge_=lc.challenge("Give the code reply: 'I prefer yesterday's newspaper.'",
                                    "Je préfère le journal d'hier aime la demain",
                                    ["Je préfère le journal d'hier"], "Preferences", P_CODE,
                                    hints=[H("'I prefer' = 'je préfère' (not 'aime' - that's 'like').", 1),
                                           H("'Yesterday's' = 'd'hier' - 'demain' is tomorrow.", 4),
                                           H("Word by word: I prefer / the newspaper / of yesterday.")])),
    lc.node("v_end", T.END, "Très bien... Le colis est au café, sous la table. Bonne chance.",
            "Very good... The package is at the café, under the table. Good luck.", outcome="GotLocation"),
], cover_blown_outcome="Suspicious")

kiosk_retry = lc.conversation(DATA, "DA_Conv_Kiosk_Retry", "Newspaper kiosk - second chance", paris, "r_start", [
    lc.node("r_start", T.PLAYER_CHALLENGE, "Encore vous ? Alors... le code ?", "You again? So... the code?", "r_end",
            challenge_=lc.challenge("Give the code reply: 'I prefer yesterday's newspaper.'",
                                    "Je préfère le journal d'hier aime la demain",
                                    ["Je préfère le journal d'hier"], "Preferences", P_CODE,
                                    hints=[H("'I prefer' = 'je préfère'.", 1), H("'Yesterday's' = 'd'hier'.", 4),
                                           H("I prefer / the newspaper / of yesterday.")])),
    lc.node("r_end", T.END, "Bon. Le colis est au café, sous la table. Ne me revoyez plus.",
            "Fine. The package is at the café, under the table. Don't come back.", outcome="GotLocation"),
], cover_blown_outcome="Suspicious")

# --- Mission: First Contact (tutorial) -------------------------------------------------------------------------
first_contact = lc.mission(MISSIONS, "MA_Paris_FirstContact", "Paris_FirstContact", "Operation First Contact",
    "Handler: Welcome to Paris. Blend in at the café, then meet our contact Marcel at the newspaper kiosk. "
    "He'll tell you where the package is. First, let's go over the phrases you'll need.",
    "o_cafe", [
        lc.objective("o_cafe", O.TALK_TO, "Camille", "Blend in: order a coffee from Camille at the café.",
                     [lc.transition("o_kiosk")]),
        lc.objective("o_kiosk", O.TALK_TO, "Marcel", "Meet Marcel at the newspaper kiosk and give the code phrase.",
                     [lc.transition("o_package", outcome="GotLocation"),
                      lc.transition("o_lose_tail", outcome="Suspicious")]),
        lc.objective("o_lose_tail", O.REACH_LOCATION, "Metro", "Marcel got nervous. Lose any tail: walk to the metro entrance.",
                     [lc.transition("o_kiosk")], flags=["LostTail"]),
        lc.objective("o_package", O.COLLECT, "Package", "Pick up the package under the café table.",
                     [lc.transition("MissionComplete")]),
    ],
    chunks=[
        lc.chunk("Bonjour", "Hello / Good morning", "You walk up to someone.", "Greetings"),
        lc.chunk("je voudrais", "I would like (polite)", "You want to order something politely.", "Ordering"),
        lc.chunk("s'il vous plaît", "please (polite)", "You're asking a stranger for something.", "Politeness"),
        lc.chunk("Merci", "Thank you", "Someone hands you your coffee.", "Politeness"),
        lc.chunk("Je suis en vacances", "I'm on holiday", "Someone asks why you're in Paris.", "SmallTalk"),
        lc.chunk("Où est la gare ?", "Where is the station?", "You need to find the train station.", "Directions"),
        lc.chunk("à gauche", "to the left", "Someone points left.", "Directions"),
        lc.chunk("le journal d'hier", "yesterday's newspaper", "You give Marcel the code phrase.", "Preferences"),
    ],
    can_dos=["Greet someone and order a drink politely (A1)",
             "Say why you are in a city - on holiday or for work (A1)",
             "Ask where a place is and follow simple directions (A1)"],
    debrief=4)

FRENCH_NOTES = [
    lc.grammar_note("fr_voudrais", "Je voudrais = I would like",
                    "'Je voudrais' is the polite way to ask for things. 'Je veux' (I want) is correct French but sounds "
                    "pushy with strangers.\nJe voudrais un café. / Je voudrais la baguette du matin.",
                    ["voudrais"], 2),
    lc.grammar_note("fr_vous", "Vous: the polite 'you'",
                    "French has two words for 'you'. Use 'vous' with strangers, shopkeepers and anyone older; "
                    "'tu' is for friends and kids.\nS'il vous plaît (please, polite). Merci, vous aussi (thanks, you too).",
                    ["vous"], 2),
    lc.grammar_note("fr_ou_est", "Où est...? = Where is...?",
                    "Put the place after 'Où est': Où est la gare ? Où est le pont ?\n"
                    "'la' goes with feminine nouns (la gare), 'le' with masculine ones (le pont).",
                    ["où"], 2),
    lc.grammar_note("fr_je_suis", "Je suis = I am",
                    "'Être' (to be): je suis = I am, vous êtes = you are.\nJe suis en vacances. Je suis ici pour le travail. "
                    "Je suis pressé.",
                    ["suis"], 2),
    lc.grammar_note("fr_prices", "Prices: euros first, then cents",
                    "Say the euros, then the cents, without 'and': deux euros cinquante = €2.50, un euro vingt = €1.20.\n"
                    "'Voilà' = here you go.",
                    ["euros", "euro"], 2),
    lc.grammar_note("fr_aller_futur", "Il va pleuvoir = it's going to rain",
                    "Near future: aller (to go) + an infinitive, just like English 'going to'.\n"
                    "Il va pleuvoir (it's going to rain). Je vais payer (I'm going to pay).",
                    ["va"], 1),
]

paris = lc.language_pack("/Game/LinguaSpy/Data/French", "DA_Lang_French_Paris", "Paris", "French", "Paris", "fr",
                         "Croissants, cafés and coded whispers.", "8FB8DE", 0,
                         city_map="/Game/LinguaSpy/Maps/Lvl_Paris",
                         missions=[first_contact], conversations=[cafe, kiosk, kiosk_retry],
                         grammar_notes=FRENCH_NOTES, allow_typing=True, special_chars="éèêàâçùûîôœ")

print("Paris content built:", [a.get_name() for a in (cafe, kiosk, kiosk_retry, first_contact, paris)])
