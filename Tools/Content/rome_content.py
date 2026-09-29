"""Rome / Italian: 'Protocollo Espresso' (3 conversations). Script: docs/Rome_Mission_Script.md (review later).
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/rome_content.py", encoding="utf-8").read())
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

T, O = lc.T, lc.O
EAL = unreal.EditorAssetLibrary
DATA = "/Game/LinguaSpy/Data/Italian"
MAP = "/Game/LinguaSpy/Maps/Lvl_Rome"

pack = lc.language_pack(DATA, "DA_Lang_Italian_Rome", "Rome", "Italian", "Rome", "it",
                        "Espresso at the bar, secrets by the fountain.", "E07A5F", 3)

bar = lc.conversation(DATA, "DA_Conv_Rome_Bar", "Bar - an espresso like a Roman", pack, "e1", [
    lc.node("e1", T.PLAYER_CHALLENGE, "Buongiorno! Cosa prende?", "Good morning! What will you have?", "e2",
            challenge_=lc.challenge("Say good morning and order a coffee, please.",
                                    "Buongiorno un caffè per favore tè grazie due",
                                    ["Buongiorno un caffè per favore", "Un caffè per favore"], "Ordering")),
    lc.node("e2", T.PLAYER_CHOICE, "Al banco o al tavolo?", "At the counter or at a table?", "", choices=[
        lc.choice("At the counter (like a local).", lc.challenge("Say: 'At the counter, thanks.'", "Al banco grazie tavolo per",
                                                                 ["Al banco grazie"], "Ordering"), "e3", flags=["LocalStyle"]),
        lc.choice("At a table.", lc.challenge("Say: 'At a table, please.'", "Al tavolo per favore banco grazie",
                                              ["Al tavolo per favore"], "Ordering"), "e3"),
    ]),
    lc.node("e3", T.PLAYER_CHALLENGE, "Sono un euro e venti.", "That's one euro twenty.", "e4",
            challenge_=lc.challenge("Pay: 'Here you are.'", "Ecco a lei grazie tu", ["Ecco a lei"], "Shopping")),
    lc.node("e4", T.END, "Grazie! Ah... un signore la cerca vicino alla fontana.",
            "Thanks! Ah... a gentleman is looking for you near the fountain.", outcome="GotHint"),
])

street = lc.conversation(DATA, "DA_Conv_Rome_Giulia", "Piazza - asking the way", pack, "g1", [
    lc.node("g1", T.PLAYER_CHALLENGE, "Posso aiutarla?", "Can I help you?", "g2",
            challenge_=lc.challenge("Excuse yourself and ask where the fountain is.", "Scusi dov'è la fontana il museo dove",
                                    ["Scusi dov'è la fontana", "Dov'è la fontana"], "Directions")),
    lc.node("g2", T.NPC_LINE, "Sempre dritto, poi a sinistra.", "Straight on, then left.", "g3"),
    lc.node("g3", T.PLAYER_CHALLENGE, "Chiaro?", "Clear?", "g4",
            challenge_=lc.challenge("Thank her a lot.", "Grazie mille prego ciao", ["Grazie mille"], "Politeness")),
    lc.node("g4", T.END, "Prego! Buona giornata.", "You're welcome! Have a nice day.", outcome="GotDirections"),
])

contact = lc.conversation(DATA, "DA_Conv_Rome_Marco", "Fountain - the contact", pack, "m1", [
    lc.node("m1", T.PLAYER_CHALLENGE, "Bella giornata, vero?",
            "Lovely day, isn't it? (Code! Reply: 'Yes, but tomorrow it will rain.')", "m2",
            challenge_=lc.challenge("Give the code reply: 'Yes, but tomorrow it will rain.'", "Sì ma domani pioverà oggi nevicherà",
                                    ["Sì ma domani pioverà"], "Code", penalty=45.0)),
    lc.node("m2", T.PLAYER_CHALLENGE, "Ha una moneta per la fontana?", "Do you have a coin for the fountain?", "m3",
            challenge_=lc.challenge("Say: 'Yes, here it is.'", "Sì eccola no ecco", ["Sì eccola"], "Code", penalty=45.0)),
    lc.node("m3", T.END, "Perfetto. I documenti sono al Colosseo. Buona fortuna.",
            "Perfect. The documents are at the Colosseum. Good luck.", outcome="MissionDone"),
], cover_blown_outcome="Burned")

mission = lc.mission(DATA + "/Missions", "MA_Rome_ProtocolloEspresso", "Rome_ProtocolloEspresso", "Protocollo Espresso",
    "Handler: In Rome, you drink your espresso standing at the bar. Blend in, find out where the fountain is, "
    "and give our contact Marco the code.",
    "o_bar", [
        lc.objective("o_bar", O.TALK_TO, "Luca", "Order an espresso from Luca at the bar.", [lc.transition("o_giulia", outcome="GotHint")]),
        lc.objective("o_giulia", O.TALK_TO, "Giulia", "Ask Giulia the way to the fountain.", [lc.transition("o_fountain", outcome="GotDirections")]),
        lc.objective("o_fountain", O.REACH_LOCATION, "Fountain", "Walk to the fountain.", [lc.transition("o_marco")]),
        lc.objective("o_marco", O.TALK_TO, "Marco", "Give Marco the code phrase.",
                     [lc.transition("MissionComplete", outcome="MissionDone"), lc.transition("MissionFailed", outcome="Burned")]),
    ])

pack = lc.language_pack(DATA, "DA_Lang_Italian_Rome", "Rome", "Italian", "Rome", "it",
                        "Espresso at the bar, secrets by the fountain.", "E07A5F", 3,
                        city_map=MAP if EAL.does_asset_exist(MAP) else "", missions=[mission], conversations=[bar, street, contact])
print("Rome content:", [a.get_name() for a in (bar, street, contact, mission, pack)])
