"""Madrid mission 2: 'El Palacio' (A2; draft in docs/Phase5_Scripts.md, review later).
Run AFTER madrid_content.py; rewrites the Madrid pack with both missions. Then rebuild the map (cities.MADRID has the
new NPCs/marker/pickup) and run voice_build for Spanish once a Spanish voice is set up.
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/madrid_palacio.py", encoding="utf-8").read())
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

T, O = lc.T, lc.O
H = lc.hint
EAL = unreal.EditorAssetLibrary
DATA = "/Game/LinguaSpy/Data/Spanish"
MISSIONS = DATA + "/Missions"
MAP = "/Game/LinguaSpy/Maps/Lvl_Madrid"
M2 = "Madrid_Palacio"
P = 20.0
P_CODE = 25.0

pack = EAL.load_asset(DATA + "/DA_Lang_Spanish_Madrid")

javier2 = lc.conversation(DATA, "DA_Conv_Javier_Palacio", "Tapas bar - what did you do yesterday?", pack, "j1", [
    lc.node("j1", T.PLAYER_CHALLENGE, "¡Hombre, otra vez por aquí! ¿Qué hizo ayer?", "Hey, you again! What did you do yesterday?", "j2",
            challenge_=lc.challenge("Say: 'Yesterday I visited the museum.'", "Ayer visité el museo visito mañana parque",
                                    ["Ayer visité el museo"], "Past", P,
                                    hints=[H("'Yesterday' = 'ayer' ('mañana' is tomorrow).", 0),
                                           H("Past tense: 'visité' = I visited ('visito' = I visit).", 1)])),
    lc.node("j2", T.PLAYER_CHOICE, "¿Y le gustó?", "And did you like it?", "", choices=[
        lc.choice("Say you loved it.", lc.challenge("Say: 'Yes, I liked it a lot.'", "Sí me gustó mucho gusta poco",
                                                    ["Sí me gustó mucho"], "Past", P,
                                                    hints=[H("Past: 'me gustó' = I liked it ('me gusta' = I like it).", 2)]), "j3"),
        lc.choice("Say it was boring.", lc.challenge("Say: 'Not much, it was boring.'", "No mucho era aburrido es divertido",
                                                     ["No mucho era aburrido"], "Past", P,
                                                     hints=[H("'It was' (description) = 'era'.", 2)]), "j3"),
    ]),
    lc.node("j3", T.END, "Mire, el palacio abre pronto. La taquilla está al lado.",
            "Look, the palace opens soon. The ticket booth is right next to it.", outcome="GotTip"),
])

rosa = lc.conversation(DATA, "DA_Conv_Madrid_Rosa", "Taquilla - tickets", pack, "r1", [
    lc.node("r1", T.PLAYER_CHALLENGE, "Buenos días. ¿Cuántas entradas?", "Good morning. How many tickets?", "r2",
            challenge_=lc.challenge("Ask for two tickets, please.", "Dos entradas por favor tres entrada gracias",
                                    ["Dos entradas por favor"], "Numbers", P,
                                    hints=[H("Two = 'dos'.", 0), H("Plural: 'entradas'.", 1)])),
    lc.node("r2", T.PLAYER_CHALLENGE, "¿Algo más?", "Anything else?", "r3",
            challenge_=lc.challenge("Ask: 'What time does the palace open?'", "A qué hora abre el palacio cierra cuándo",
                                    ["A qué hora abre el palacio"], "Time", P,
                                    hints=[H("'At what time' = 'a qué hora'.", 1), H("'Opens' = 'abre' ('cierra' = closes).", 3)])),
    lc.intel("r3", "Abre a las diez y cierra a las seis.", "It opens at ten and closes at six.", "r4",
             "When does the palace open?", ["9:00", "10:00", "6:00"], 1, flag="KnowsOpening", skill="Time", cost=6.0),
    lc.node("r4", T.PLAYER_CHOICE, "Son veinticuatro euros. ¿En efectivo o con tarjeta?", "That's twenty-four euros. Cash or card?", "",
            choices=[
                lc.choice("Pay by card.", lc.challenge("Say: 'By card, please.'", "Con tarjeta por favor efectivo en",
                                                       ["Con tarjeta por favor"], "Shopping", P,
                                                       hints=[H("'By card' = 'con tarjeta'.", 0)]), "r5"),
                lc.choice("Pay cash.", lc.challenge("Say: 'In cash, here you are.'", "En efectivo aquí tiene tarjeta con",
                                                    ["En efectivo aquí tiene"], "Shopping", P,
                                                    hints=[H("'In cash' = 'en efectivo'.", 0), H("'Here you are' = 'aquí tiene'.", 2)]), "r5"),
            ]),
    lc.node("r5", T.END, "Gracias. ¡Que disfrute la visita!", "Thanks. Enjoy the visit!", outcome="GotTickets"),
])

alvaro = lc.conversation(DATA, "DA_Conv_Madrid_Alvaro", "Palace gate - the guard", pack, "a1", [
    lc.node("a1", T.PLAYER_CHALLENGE, "Buenos días. ¿De dónde viene?", "Good morning. Where are you coming from?", "a2",
            challenge_=lc.challenge("Say: 'I'm a tourist, I come from London.'", "Soy turista vengo de Londres voy a",
                                    ["Soy turista vengo de Londres"], "SmallTalk", P,
                                    hints=[H("'I am' (identity) = 'soy'.", 0), H("'I come from' = 'vengo de'.", 2)])),
    lc.node("a2", T.PLAYER_CHALLENGE, "¿Y qué quiere ver?", "And what do you want to see?", "a3",
            challenge_=lc.challenge("Say: 'I want to see the throne room.'", "Quiero ver la sala del trono quiere cocina",
                                    ["Quiero ver la sala del trono"], "Preferences", P,
                                    hints=[H("'I want' = 'quiero' ('quiere' = he/she/you want).", 0)])),
    lc.node("a3", T.END, "Adelante.", "Go ahead.", outcome="Inside"),
], cover_blown_outcome="Suspicious")

alvaro_retry = lc.conversation(DATA, "DA_Conv_Madrid_Alvaro_Retry", "Palace gate - second try", pack, "x1", [
    lc.node("x1", T.PLAYER_CHALLENGE, "¿Otra vez usted? ¿Tiene entrada?", "You again? Do you have a ticket?", "x2",
            challenge_=lc.challenge("Say: 'Yes, I have two tickets.'", "Sí tengo dos entradas tiene una",
                                    ["Sí tengo dos entradas"], "Numbers", P,
                                    hints=[H("'I have' = 'tengo' ('tiene' = you have, polite).", 1)])),
    lc.node("x2", T.END, "Vale. Adelante.", "OK. Go ahead.", outcome="Inside"),
], cover_blown_outcome="Suspicious")

carmen2 = lc.conversation(DATA, "DA_Conv_Carmen_Palacio", "Plaza - asking for help", pack, "h1", [
    lc.node("h1", T.PLAYER_CHALLENGE, "¿Está perdido?", "Are you lost?", "h2",
            challenge_=lc.challenge("Say yes and ask: 'Can you help me?'", "Sí me puede ayudar puedo ayuda",
                                    ["Sí me puede ayudar"], "Help", P,
                                    hints=[H("Polite 'can you' = 'puede' (usted).", 2), H("'Help me' = 'me ... ayudar'.")])),
    lc.node("h2", T.END, "Claro. Diga al guardia que tiene entradas.", "Of course. Tell the guard you have tickets.", outcome="Helped"),
])

diego2 = lc.conversation(DATA, "DA_Conv_Diego_Palacio", "Palace - the handover", pack, "d1", [
    lc.node("d1", T.PLAYER_CHALLENGE, "¿Tiene la carpeta?", "Do you have the folder?", "d2",
            challenge_=lc.challenge("Say: 'Yes, here is the folder.'", "Sí aquí está la carpeta el libro",
                                    ["Sí aquí está la carpeta"], "Code", P_CODE,
                                    hints=[H("'Here is' = 'aquí está'.", 1)])),
    lc.node("d2", T.PLAYER_CHALLENGE, "¿Y cuándo sale de Madrid?", "And when do you leave Madrid?", "d3",
            challenge_=lc.challenge("Say: 'Tomorrow I'm going to leave Madrid.'", "Mañana voy a salir de Madrid ayer fui",
                                    ["Mañana voy a salir de Madrid"], "Future", P_CODE,
                                    hints=[H("'Tomorrow' = 'mañana'.", 0),
                                           H("Near future: 'voy a' + infinitive ('fui' is past).", 1)])),
    lc.node("d3", T.END, "Perfecto. Buen viaje, agente.", "Perfect. Safe travels, agent.", outcome="MissionDone"),
], cover_blown_outcome="Burned")

palacio = lc.mission(MISSIONS, "MA_Madrid_Palacio", M2, "El Palacio",
    "Handler: Diego says the folder is inside the palace. Get tickets, get past the guard, find the folder in the arcade "
    "and hand it to Diego. This time you'll need to talk about yesterday and tomorrow.",
    "o_javier", [
        lc.objective("o_javier", O.TALK_TO, "Javier", "Catch up with Javier at the tapas bar.",
                     [lc.transition("o_rosa", outcome="GotTip")]),
        lc.objective("o_rosa", O.TALK_TO, "Rosa", "Buy two tickets from Rosa at the taquilla.",
                     [lc.transition("o_gate", outcome="GotTickets")]),
        lc.objective("o_gate", O.REACH_LOCATION, "PalaceGate", "Walk to the palace gate.", [lc.transition("o_alvaro")]),
        lc.objective("o_alvaro", O.TALK_TO, "Alvaro", "Get past Álvaro, the guard.",
                     [lc.transition("o_folder", outcome="Inside"), lc.transition("o_carmen", outcome="Suspicious")]),
        lc.objective("o_carmen", O.TALK_TO, "Carmen", "The guard is suspicious. Ask Carmen in the plaza for help.",
                     [lc.transition("o_alvaro", outcome="Helped")], flags=["AskedCarmen"]),
        lc.objective("o_folder", O.COLLECT, "Folder", "Find the folder in the arcade.", [lc.transition("o_diego")]),
        lc.objective("o_diego", O.TALK_TO, "Diego", "Hand the folder to Diego.",
                     [lc.transition("MissionComplete", outcome="MissionDone"), lc.transition("MissionFailed", outcome="Burned")]),
    ],
    chunks=[
        lc.chunk("Ayer visité", "Yesterday I visited", "Someone asks what you did yesterday.", "Past"),
        lc.chunk("dos entradas", "two tickets", "You're at a ticket booth with a friend.", "Numbers"),
        lc.chunk("¿A qué hora abre?", "What time does it open?", "You want to know when a place opens.", "Time"),
        lc.chunk("con tarjeta", "by card", "You want to pay with your card.", "Shopping"),
        lc.chunk("vengo de Londres", "I come from London", "A guard asks where you're from.", "SmallTalk"),
        lc.chunk("¿Me puede ayudar?", "Can you help me? (polite)", "You're lost and ask a stranger.", "Help"),
        lc.chunk("Mañana voy a", "Tomorrow I'm going to", "You talk about your plans.", "Future"),
        lc.chunk("la carpeta", "the folder", "Diego asks for the folder.", "Code"),
    ],
    can_dos=["Talk about what you did yesterday (A2)",
             "Buy tickets and ask about opening times (A2)",
             "Ask a stranger for help (A2)"],
    debrief=5)

SPANISH_NOTES = [
    lc.grammar_note("es_preterite", "Ayer visité: the past for finished actions",
                    "For something done and finished, Spanish uses the preterite: visité (I visited), me gustó (I liked it).\n"
                    "Ayer visité el museo. Sí, me gustó mucho.", ["visité", "gustó"], 2),
    lc.grammar_note("es_ir_a", "Voy a + infinitive = going to",
                    "Near future, just like English 'going to': voy a salir (I'm going to leave), vamos a comer (we're going to eat).",
                    ["voy"], 1),
    lc.grammar_note("es_usted", "Usted: the polite 'you'",
                    "With strangers, use usted and the 'he/she' verb form: ¿Me puede ayudar? (Can you help me?), ¿Qué quiere ver?",
                    ["puede"], 1),
]

mission1 = EAL.load_asset(MISSIONS + "/MA_Madrid_TapaSecreta")
existing = [EAL.load_asset(DATA + "/" + n) for n in ("DA_Conv_TapasBar", "DA_Conv_Madrid_Carmen", "DA_Conv_Madrid_Diego")]
pack = lc.language_pack(DATA, "DA_Lang_Spanish_Madrid", "Madrid", "Spanish", "Madrid", "es",
                        "Tapas, plazas and passwords at siesta time.", "F2A65A", 1,
                        city_map=MAP, missions=[mission1, palacio],
                        conversations=existing + [javier2, rosa, alvaro, alvaro_retry, carmen2, diego2],
                        grammar_notes=SPANISH_NOTES, allow_typing=True, special_chars="áéíóúñ¿¡")
print("El Palacio built:", [a.get_name() for a in (javier2, rosa, alvaro, alvaro_retry, carmen2, diego2, palacio)])
