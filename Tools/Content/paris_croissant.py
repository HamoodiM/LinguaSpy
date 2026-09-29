"""Paris mission 2: Opération Croissant (approved draft: docs/Paris_Mission_Draft.md).
Run AFTER paris_content.py (it reuses First Contact and rewrites the Paris pack with both missions):
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/paris_croissant.py", encoding="utf-8").read())
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

T, O = lc.T, lc.O
EAL = unreal.EditorAssetLibrary
DATA = "/Game/LinguaSpy/Data/French"
MISSIONS = "/Game/LinguaSpy/Data/French/Missions"
M2 = "Paris_Croissant"

paris = EAL.load_asset(DATA + "/DA_Lang_French_Paris")

cafe2 = lc.conversation(DATA, "DA_Conv_Cafe_Croissant", "Café - the message", paris, "c1", [
    lc.node("c1", T.PLAYER_CHALLENGE, "Bonjour ! Qu'est-ce que je vous sers ?", "Hello! What can I get you?", "c2",
            challenge_=lc.challenge("Greet her and politely order a coffee.",
                                    "Bonjour je voudrais un café s'il vous plaît veux thé merci",
                                    ["Bonjour je voudrais un café s'il vous plaît", "Bonjour un café s'il vous plaît"], "Ordering")),
    lc.node("c2", T.PLAYER_CHOICE, "Vous êtes touriste ?", "Are you a tourist?", "", choices=[
        lc.choice("Play the tourist: say you're on holiday.",
                  lc.challenge("Say: yes, I'm on holiday.", "Oui je suis en vacances travail non", ["Oui je suis en vacances"], "SmallTalk"),
                  "c3", flags=["CoverTourist"]),
        lc.choice("Stay vague: say you're here for work.",
                  lc.challenge("Say: no, I'm here for work.", "Non je suis ici pour le travail vacances oui",
                               ["Non je suis ici pour le travail"], "SmallTalk"),
                  "c3", flags=["CoverWork"]),
    ]),
    lc.node("c3", T.PLAYER_CHALLENGE, "Un grand ou un petit ?", "A large or a small?", "c4",
            challenge_=lc.challenge("Ask for a small one, please.", "Un petit s'il vous plaît grand deux",
                                    ["Un petit s'il vous plaît", "Petit s'il vous plaît"], "Ordering")),
    lc.node("c4", T.PLAYER_CHALLENGE, "Ça fait deux euros cinquante.", "That's two euros fifty.", "c5",
            challenge_=lc.challenge("Hand over the money: 'Here's two euros fifty.'", "Voilà deux euros cinquante trois quinze",
                                    ["Voilà deux euros cinquante"], "Numbers")),
    lc.node("c5", T.END, "Merci ! Oh, et... un monsieur a laissé ce message pour vous.",
            "Thanks! Oh, and... a gentleman left this message for you.", outcome="GotNote"),
])

kiosk2 = lc.conversation(DATA, "DA_Conv_Kiosk_Croissant", "Kiosk - where is the package?", paris, "k1", [
    lc.node("k1", T.PLAYER_CHALLENGE, "Bonjour ! Vous cherchez quelque chose ?", "Hello! Looking for something?", "k2",
            challenge_=lc.challenge("Say hello and ask where the station is.", "Bonjour où est la gare s'il vous plaît le musée qui",
                                    ["Bonjour où est la gare", "Bonjour où est la gare s'il vous plaît",
                                     "Bonjour s'il vous plaît où est la gare"], "Directions")),
    lc.node("k2", T.PLAYER_CHALLENGE, "Vous aimez les journaux d'aujourd'hui ?",
            "Do you like today's newspapers? (Code phrase! Reply: 'I prefer yesterday's newspaper.')", "k3",
            challenge_=lc.challenge("Give the code reply: 'I prefer yesterday's newspaper.'", "Je préfère le journal d'hier aime la demain",
                                    ["Je préfère le journal d'hier"], "Code", penalty=40.0)),
    lc.node("k3", T.END, "Très bien. Le colis est chez le boulanger, rue Lepic. Demandez « la baguette du matin ».",
            "Very good. The package is at the baker's on rue Lepic. Ask for \"the morning baguette\".", outcome="GotBakery"),
], cover_blown_outcome="Suspicious")

metro = lc.conversation(DATA, "DA_Conv_Metro_Luc", "Metro booth - backup contact", paris, "m1", [
    lc.node("m1", T.PLAYER_CHALLENGE, "Bonjour, je peux vous aider ?", "Hello, can I help you?", "m2",
            challenge_=lc.challenge("Ask for a ticket, please.", "Un ticket s'il vous plaît deux billet merci",
                                    ["Un ticket s'il vous plaît"], "Ordering")),
    lc.node("m2", T.PLAYER_CHALLENGE, "Voilà. Deux euros dix.", "Here. Two euros ten.", "m3",
            challenge_=lc.challenge("Pay: 'Here's two euros ten.'", "Voilà deux euros dix douze vingt",
                                    ["Voilà deux euros dix"], "Numbers")),
    lc.node("m3", T.PLAYER_CHALLENGE, "Votre ami Marcel a téléphoné. Allez à la boulangerie, rue Lepic.",
            "Your friend Marcel called. Go to the bakery on rue Lepic.", "m4",
            challenge_=lc.challenge("Thank him and say goodbye.", "Merci beaucoup au revoir bonjour bien",
                                    ["Merci beaucoup au revoir"], "Politeness")),
    lc.node("m4", T.END, "Et demandez « la baguette du matin ».", "And ask for \"the morning baguette\".", outcome="GotBakery"),
])

bakery = lc.conversation(DATA, "DA_Conv_Bakery_Sophie", "Boulangerie - the morning baguette", paris, "b1", [
    lc.node("b1", T.PLAYER_CHALLENGE, "Bonjour ! Vous désirez ?", "Hello! What would you like?", "b2",
            challenge_=lc.challenge("Ask for 'the morning baguette'.", "Je voudrais la baguette du matin soir le croissant",
                                    ["Je voudrais la baguette du matin"], "Ordering")),
    lc.node("b2", T.PLAYER_CHALLENGE, "Ah... Une seule ?", "Ah... Just one?", "b3",
            challenge_=lc.challenge("Yes, just one, please.", "Oui une seule s'il vous plaît deux non",
                                    ["Oui une seule s'il vous plaît", "Une seule s'il vous plaît"], "Numbers")),
    lc.node("b3", T.PLAYER_CHOICE, "Ça fait un euro vingt.", "That's one euro twenty.", "", choices=[
        lc.choice("Pay cash.", lc.challenge("Say: 'Here's one euro twenty.'", "Voilà un euro vingt deux trente",
                                            ["Voilà un euro vingt"], "Numbers"), "b5"),
        lc.choice("Ask to pay by card.", lc.challenge("Ask: 'Can I pay by card?'", "Je peux payer par carte espèces",
                                                      ["Je peux payer par carte"], "Shopping"), "b4"),
    ]),
    lc.node("b4", T.PLAYER_CHALLENGE, "Non, désolée, seulement en espèces.", "No, sorry, cash only.", "b5",
            challenge_=lc.challenge("Say: 'OK, here's one euro twenty.'", "D'accord voilà un euro vingt carte deux",
                                    ["D'accord voilà un euro vingt"], "Numbers")),
    lc.node("b5", T.END, "Merci. Attention... elle est fragile, cette baguette.", "Thanks. Careful... this baguette is fragile.",
            outcome="GotPackage"),
])

painter = lc.conversation(DATA, "DA_Conv_Painter_Hugo", "Place du Tertre - the painter", paris, "p1", [
    lc.node("p1", T.PLAYER_CHOICE, "Vous aimez ma peinture ?", "Do you like my painting?", "", choices=[
        lc.choice("Compliment it.", lc.challenge("Say: 'Yes, it's very beautiful.'", "Oui c'est très beau laid lent",
                                                 ["Oui c'est très beau"], "SmallTalk"), "p2", flags=["ComplimentedHugo"]),
        lc.choice("Brush him off.", lc.challenge("Say: 'Sorry, I'm in a hurry.'", "Désolé je suis pressé lent laid",
                                                 ["Désolé je suis pressé"], "SmallTalk"), "p2"),
    ]),
    lc.node("p2", T.PLAYER_CHALLENGE, "Je peux vous aider ?", "Can I help you?", "p3",
            challenge_=lc.challenge("Ask: 'What time is it?'", "Quelle heure est-il quel temps où",
                                    ["Quelle heure est-il"], "Time")),
    lc.node("p3", T.NPC_LINE, "Il est midi moins dix.", "It's ten to twelve.", "p4"),
    lc.node("p4", T.PLAYER_CHALLENGE, "Autre chose ?", "Anything else?", "p5",
            challenge_=lc.challenge("Ask where the bridge is, please.", "Où est le pont s'il vous plaît la gare quand",
                                    ["Où est le pont s'il vous plaît", "Où est le pont"], "Directions")),
    lc.node("p5", T.END, "Le pont ? Tout droit, puis à droite, après l'église.",
            "The bridge? Straight on, then right, after the church.", outcome="GotDirections"),
])

bridge = lc.conversation(DATA, "DA_Conv_Bridge_Elise", "The bridge - the handover", paris, "e1", [
    lc.node("e1", T.PLAYER_CHALLENGE, "Il fait beau aujourd'hui, non ?",
            "Nice weather today, isn't it? (Code: 'Yes, but it's going to rain tonight.')", "e2",
            challenge_=lc.challenge("Give the code reply: 'Yes, but it's going to rain tonight.'",
                                    "Oui mais il va pleuvoir ce soir demain neiger matin",
                                    ["Oui mais il va pleuvoir ce soir"], "Code", penalty=50.0)),
    lc.node("e2", T.PLAYER_CHOICE, "Vous avez quelque chose pour moi ?", "Do you have something for me?", "", choices=[
        lc.choice("Hand over the baguette.", lc.challenge("Say: 'Yes, here's the baguette.'", "Oui voici la baguette pain quoi",
                                                          ["Oui voici la baguette"], "Code", penalty=50.0), "e4"),
        lc.choice("Test her first: ask for the password.", lc.challenge("Ask: 'What's the password?'",
                                                                        "Quel est le mot de passe quoi pain",
                                                                        ["Quel est le mot de passe"], "Code", penalty=50.0), "e3"),
    ]),
    lc.node("e3", T.PLAYER_CHALLENGE, "« Croissant. »", "\"Croissant.\"", "e4",
            challenge_=lc.challenge("Say: 'Yes, here's the baguette.'", "Oui voici la baguette pain quoi",
                                    ["Oui voici la baguette"], "Code", penalty=50.0)),
    lc.node("e4", T.PLAYER_CHALLENGE, "Parfait. Le train part à quelle heure ?",
            "Perfect. What time does the train leave? (Tell her: 2 p.m.)", "e5",
            challenge_=lc.challenge("Say: 'At two p.m.' (14h)", "À quatorze heures quatre deux midi",
                                    ["À quatorze heures"], "Time", penalty=50.0)),
    lc.node("e5", T.END, "Merci, agent. La France vous remercie.", "Thank you, agent. France thanks you.", outcome="MissionDone"),
], cover_blown_outcome="Burned")

croissant = lc.mission(MISSIONS, "MA_Paris_Croissant", M2, "Opération Croissant",
    "Handler: Agent, a courier is moving a microfilm through Paris today. Our contact Marcel knows where it is. "
    "Get it to Élise on the bridge before the other side does, and don't let anyone hear an accent.",
    "o_cafe", [
        lc.objective("o_cafe", O.TALK_TO, "Camille", "Stop at the café: Camille may have a message for you.",
                     [lc.transition("o_kiosk", outcome="GotNote")]),
        lc.objective("o_kiosk", O.TALK_TO, "Marcel", "Ask Marcel at the kiosk where the package is.",
                     [lc.transition("o_bakery", outcome="GotBakery"), lc.transition("o_metro", outcome="Suspicious")]),
        lc.objective("o_metro", O.TALK_TO, "Luc", "Marcel panicked. Find the backup contact at the metro ticket booth.",
                     [lc.transition("o_bakery", outcome="GotBakery")]),
        lc.objective("o_bakery", O.TALK_TO, "Sophie", "Go to the boulangerie and ask for \"the morning baguette\".",
                     [lc.transition("o_painter", outcome="GotPackage")]),
        lc.objective("o_painter", O.TALK_TO, "Hugo", "Ask the painter on the square for the time and the way to the bridge.",
                     [lc.transition("o_bridge", outcome="GotDirections")]),
        lc.objective("o_bridge", O.REACH_LOCATION, "Bridge", "Head to the bridge over the Seine.",
                     [lc.transition("o_elise")]),
        lc.objective("o_elise", O.TALK_TO, "Elise", "Meet Élise on the bridge and hand over the baguette.",
                     [lc.transition("MissionComplete", outcome="MissionDone"), lc.transition("MissionFailed", outcome="Burned")]),
    ])

first_contact = EAL.load_asset(MISSIONS + "/MA_Paris_FirstContact")
existing = [EAL.load_asset(DATA + "/" + n) for n in ("DA_Conv_Cafe", "DA_Conv_Kiosk", "DA_Conv_Kiosk_Retry")]
PARIS_MAP = "/Game/LinguaSpy/Maps/Lvl_Paris"
paris = lc.language_pack(DATA, "DA_Lang_French_Paris", "Paris", "French", "Paris", "fr",
                         "Croissants, cafés and coded whispers.", "8FB8DE", 0,
                         city_map=PARIS_MAP,
                         missions=[first_contact, croissant],
                         conversations=existing + [cafe2, kiosk2, metro, bakery, painter, bridge])

print("Croissant built:", [c.get_name() for c in (cafe2, kiosk2, metro, bakery, painter, bridge, croissant)])
