"""Madrid / Spanish (Spain): 'La Tapa Secreta' (3 conversations). Script: docs/Madrid_Mission_Script.md (review later).
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/madrid_content.py", encoding="utf-8").read())
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

T, O = lc.T, lc.O
EAL = unreal.EditorAssetLibrary
DATA = "/Game/LinguaSpy/Data/Spanish"
MAP = "/Game/LinguaSpy/Maps/Lvl_Madrid"

pack = lc.language_pack(DATA, "DA_Lang_Spanish_Madrid", "Madrid", "Spanish", "Madrid", "es",
                        "Tapas, plazas and passwords at siesta time.", "F2A65A", 1)

bar = lc.conversation(DATA, "DA_Conv_TapasBar", "Tapas bar - order like a local", pack, "t1", [
    lc.node("t1", T.PLAYER_CHALLENGE, "¡Hola! ¿Qué le pongo?", "Hi! What can I get you?", "t2",
            challenge_=lc.challenge("Say hi and order a tapa of tortilla, please.",
                                    "Hola una tapa de tortilla por favor dos jamón gracias",
                                    ["Hola una tapa de tortilla por favor", "Una tapa de tortilla por favor"], "Ordering")),
    lc.node("t2", T.PLAYER_CHOICE, "¿Algo para beber?", "Anything to drink?", "", choices=[
        lc.choice("Order a water.", lc.challenge("Say: 'A water, please.'", "Un agua por favor una café",
                                                 ["Un agua por favor"], "Ordering"), "t3"),
        lc.choice("Order a milky coffee.", lc.challenge("Say: 'A coffee with milk, please.'", "Un café con leche por favor agua sin",
                                                        ["Un café con leche por favor"], "Ordering"), "t3"),
    ]),
    lc.node("t3", T.PLAYER_CHALLENGE, "Son cuatro euros.", "That's four euros.", "t4",
            challenge_=lc.challenge("Pay: 'Here you are, four euros.'", "Aquí tiene cuatro euros cinco tres",
                                    ["Aquí tiene cuatro euros"], "Numbers")),
    lc.node("t4", T.END, "¡Gracias! Por cierto... alguien le espera cerca del metro.",
            "Thanks! By the way... someone is waiting for you near the metro.", outcome="GotHint"),
])

street = lc.conversation(DATA, "DA_Conv_Madrid_Carmen", "Plaza - asking the way", pack, "d1", [
    lc.node("d1", T.PLAYER_CHALLENGE, "¿Necesita ayuda?", "Do you need help?", "d2",
            challenge_=lc.challenge("Say yes and ask where the metro is.", "Sí dónde está el metro la calle no",
                                    ["Sí dónde está el metro", "Dónde está el metro"], "Directions")),
    lc.node("d2", T.NPC_LINE, "Siga todo recto y luego gire a la derecha.", "Go straight ahead and then turn right.", "d3"),
    lc.node("d3", T.PLAYER_CHALLENGE, "¿Está claro?", "Is that clear?", "d4",
            challenge_=lc.challenge("Thank her very much.", "Muchas gracias buenos días", ["Muchas gracias"], "Politeness")),
    lc.node("d4", T.END, "¡De nada! Buen viaje.", "You're welcome! Have a good trip.", outcome="GotDirections"),
])

contact = lc.conversation(DATA, "DA_Conv_Madrid_Diego", "Metro entrance - the contact", pack, "f1", [
    lc.node("f1", T.PLAYER_CHALLENGE, "Hace calor hoy, ¿verdad?",
            "It's hot today, isn't it? (Code! Reply: 'Yes, but the night will be cold.')", "f2",
            challenge_=lc.challenge("Give the code reply: 'Yes, but the night will be cold.'",
                                    "Sí pero la noche será fría el día caliente",
                                    ["Sí pero la noche será fría"], "Code", penalty=45.0)),
    lc.node("f2", T.PLAYER_CHALLENGE, "Bien. ¿Tiene la llave?", "Good. Do you have the key?", "f3",
            challenge_=lc.challenge("Say: 'Yes, here it is.'", "Sí aquí está no allí", ["Sí aquí está"], "Code", penalty=45.0)),
    lc.node("f3", T.END, "Perfecto. La carpeta está en el palacio. ¡Suerte!",
            "Perfect. The folder is at the palace. Good luck!", outcome="MissionDone"),
], cover_blown_outcome="Burned")

mission = lc.mission(DATA + "/Missions", "MA_Madrid_TapaSecreta", "Madrid_TapaSecreta", "La Tapa Secreta",
    "Handler: Welcome to Madrid. Our contact Diego meets agents near the metro, but only after lunch. "
    "Eat like a local at the tapas bar, ask a madrileña for directions and give Diego the code.",
    "o_bar", [
        lc.objective("o_bar", O.TALK_TO, "Javier", "Order at the tapas bar from Javier.", [lc.transition("o_carmen", outcome="GotHint")]),
        lc.objective("o_carmen", O.TALK_TO, "Carmen", "Ask Carmen in the plaza where the metro is.", [lc.transition("o_metro", outcome="GotDirections")]),
        lc.objective("o_metro", O.REACH_LOCATION, "Metro", "Walk to the metro entrance.", [lc.transition("o_diego")]),
        lc.objective("o_diego", O.TALK_TO, "Diego", "Give Diego the code phrase.",
                     [lc.transition("MissionComplete", outcome="MissionDone"), lc.transition("MissionFailed", outcome="Burned")]),
    ])

pack = lc.language_pack(DATA, "DA_Lang_Spanish_Madrid", "Madrid", "Spanish", "Madrid", "es",
                        "Tapas, plazas and passwords at siesta time.", "F2A65A", 1,
                        city_map=MAP if EAL.does_asset_exist(MAP) else "", missions=[mission], conversations=[bar, street, contact])
print("Madrid content:", [a.get_name() for a in (bar, street, contact, mission, pack)])
