"""Tokyo / Japanese: 'Konbini Contact' (3 conversations). Tiles are word chunks (no spaces) with romaji readings.
Answers use '|' between tiles. Script: docs/Tokyo_Mission_Script.md (review later).
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/tokyo_content.py", encoding="utf-8").read())
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

T, O = lc.T, lc.O
EAL = unreal.EditorAssetLibrary
DATA = "/Game/LinguaSpy/Data/Japanese"
MAP = "/Game/LinguaSpy/Maps/Lvl_Tokyo"

pack = lc.language_pack(DATA, "DA_Lang_Japanese_Tokyo", "Tokyo", "Japanese", "Tokyo", "ja",
                        "Konbini runs and whispered keigo.", "F4A7B9", 2, word_spacing=False, readings=True)


def jc(intent, tiles, answers, skill, penalty=34.0):
    return lc.challenge(intent, tiles, answers, skill, penalty=penalty, sep="|")


konbini = lc.conversation(DATA, "DA_Conv_Konbini", "Konbini - an onigiri, please", pack, "k1", [
    lc.node("k1", T.PLAYER_CHALLENGE, "いらっしゃいませ！", "Welcome! (standard shop greeting)", "k2", reading="Irasshaimase!",
            challenge_=jc("Ask for an onigiri (rice ball), please.",
                          [("おにぎり", "onigiri"), ("を", "o"), ("ください", "kudasai"), ("が", "ga"), ("パン", "pan"), ("です", "desu")],
                          ["おにぎり|を|ください"], "Ordering")),
    lc.node("k2", T.PLAYER_CHOICE, "あたためますか？", "Shall I warm it up?", "", reading="Atatamemasu ka?", choices=[
        lc.choice("Yes, please.", jc("Say: 'Yes, please.'", [("はい", "hai"), ("おねがいします", "onegaishimasu"), ("いいえ", "iie"), ("ありがとう", "arigatou")],
                                     ["はい|おねがいします"], "Politeness"), "k3"),
        lc.choice("No, it's fine.", jc("Say: 'No, I'm fine.'", [("いいえ", "iie"), ("けっこうです", "kekkou desu"), ("はい", "hai"), ("ください", "kudasai")],
                                       ["いいえ|けっこうです"], "Politeness"), "k3"),
    ]),
    lc.node("k3", T.PLAYER_CHALLENGE, "ひゃくごじゅう円です。", "That's 150 yen.", "k4", reading="Hyaku gojuu en desu.",
            challenge_=jc("Hand over the money: 'Here you go.'", [("はい", "hai"), ("どうぞ", "douzo"), ("いいえ", "iie"), ("ください", "kudasai")],
                          ["はい|どうぞ", "どうぞ"], "Shopping")),
    lc.node("k4", T.END, "ありがとうございました！", "Thank you very much!", reading="Arigatou gozaimashita!", outcome="Completed"),
])

station = lc.conversation(DATA, "DA_Conv_Tokyo_Tanaka", "Street - where is the station?", pack, "s1", [
    lc.node("s1", T.PLAYER_CHALLENGE, "どうしましたか？", "What's the matter? / Can I help?", "s2", reading="Dou shimashita ka?",
            challenge_=jc("Excuse yourself and ask where the station is.",
                          [("すみません", "sumimasen"), ("駅", "eki"), ("は", "wa"), ("どこ", "doko"), ("ですか", "desu ka"), ("を", "o"), ("なに", "nani")],
                          ["すみません|駅|は|どこ|ですか", "駅|は|どこ|ですか"], "Directions")),
    lc.node("s2", T.NPC_LINE, "まっすぐ いって、みぎです。", "Go straight, then it's on the right.", "s3", reading="Massugu itte, migi desu."),
    lc.node("s3", T.PLAYER_CHALLENGE, "わかりましたか？", "Did you understand?", "s4", reading="Wakarimashita ka?",
            challenge_=jc("Thank him politely.", [("ありがとう", "arigatou"), ("ございます", "gozaimasu"), ("すみません", "sumimasen"), ("です", "desu")],
                          ["ありがとう|ございます"], "Politeness")),
    lc.node("s4", T.END, "いいえ、どういたしまして。", "Not at all, you're welcome.", reading="Iie, dou itashimashite.", outcome="GotDirections"),
])

shrine = lc.conversation(DATA, "DA_Conv_Tokyo_Yuki", "Shrine - the contact", pack, "y1", [
    lc.node("y1", T.PLAYER_CHALLENGE, "さくらが きれいですね。",
            "The cherry blossoms are pretty, aren't they? (Code! Reply: 'Yes, but they fall quickly.')", "y2",
            reading="Sakura ga kirei desu ne.",
            challenge_=jc("Give the code reply: 'Yes, but they fall quickly.'",
                          [("はい", "hai"), ("でも", "demo"), ("すぐ", "sugu"), ("ちります", "chirimasu"), ("いいえ", "iie"), ("ゆっくり", "yukkuri"), ("さきます", "sakimasu")],
                          ["はい|でも|すぐ|ちります"], "Code", penalty=45.0)),
    lc.node("y2", T.PLAYER_CHALLENGE, "では、これを どうぞ。", "Then, please take this.", "y3", reading="Dewa, kore o douzo.",
            challenge_=jc("Thank her politely.", [("ありがとう", "arigatou"), ("ございます", "gozaimasu"), ("すみません", "sumimasen"), ("です", "desu")],
                          ["ありがとう|ございます"], "Politeness", penalty=45.0)),
    lc.node("y3", T.END, "きをつけて。", "Be careful.", reading="Ki o tsukete.", outcome="MissionDone"),
], cover_blown_outcome="Burned")

mission = lc.mission(DATA + "/Missions", "MA_Tokyo_KonbiniContact", "Tokyo_KonbiniContact", "Konbini Contact",
    "Handler: Tokyo runs on politeness. Buy something at the konbini to blend in, ask a local the way to the station, "
    "then meet Yuki at the shrine gate and give the code.",
    "o_konbini", [
        lc.objective("o_konbini", O.TALK_TO, "Sato", "Buy an onigiri from Sato at the konbini.", [lc.transition("o_tanaka")]),
        lc.objective("o_tanaka", O.TALK_TO, "Tanaka", "Ask Mr. Tanaka the way to the station.", [lc.transition("o_shrine", outcome="GotDirections")]),
        lc.objective("o_shrine", O.REACH_LOCATION, "Shrine", "Walk to the shrine gate (torii).", [lc.transition("o_yuki")]),
        lc.objective("o_yuki", O.TALK_TO, "Yuki", "Give Yuki the code phrase.",
                     [lc.transition("MissionComplete", outcome="MissionDone"), lc.transition("MissionFailed", outcome="Burned")]),
    ])

pack = lc.language_pack(DATA, "DA_Lang_Japanese_Tokyo", "Tokyo", "Japanese", "Tokyo", "ja",
                        "Konbini runs and whispered keigo.", "F4A7B9", 2, word_spacing=False, readings=True,
                        city_map=MAP if EAL.does_asset_exist(MAP) else "", missions=[mission], conversations=[konbini, station, shrine])
print("Tokyo content:", [a.get_name() for a in (konbini, station, shrine, mission, pack)])
