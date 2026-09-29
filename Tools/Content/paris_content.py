"""Paris / French content: conversations, the 'First Contact' test mission and the language pack.
Also creates placeholder packs for Madrid, Tokyo and Rome (shown as 'Coming soon' until their maps exist).
Run inside the editor:  exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/paris_content.py", encoding="utf-8").read())
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

T, O = lc.T, lc.O
DATA = "/Game/LinguaSpy/Data/French"
MISSIONS = "/Game/LinguaSpy/Data/French/Missions"

paris = lc.language_pack("/Game/LinguaSpy/Data/French", "DA_Lang_French_Paris", "Paris", "French", "Paris", "fr",
                         "Croissants, cafés and coded whispers.", "8FB8DE", 0,
                         city_map="/Game/LinguaSpy/Maps/Lvl_Paris")

# --- Café: order a coffee, small talk choice ------------------------------------------------------------------
cafe = lc.conversation(DATA, "DA_Conv_Cafe", "Café terrace - order a coffee", paris, "c_greet", [
    lc.node("c_greet", T.PLAYER_CHALLENGE, "Bonjour ! Qu'est-ce que je vous sers ?", "Hello! What can I get you?", "c_small_talk",
            challenge_=lc.challenge("Greet her and politely order a coffee.",
                                    "Bonjour je voudrais un café s'il vous plaît veux thé merci",
                                    ["Bonjour je voudrais un café s'il vous plaît", "Bonjour un café s'il vous plaît"], "Ordering")),
    lc.node("c_small_talk", T.PLAYER_CHOICE, "Vous êtes touriste ?", "Are you a tourist?", "",
            choices=[
                lc.choice("Play the tourist: say you're on holiday.",
                          lc.challenge("Say: yes, I'm on holiday.", "Oui je suis en vacances travail non",
                                       ["Oui je suis en vacances"], "SmallTalk"),
                          "c_size", flags=["CoverTourist"]),
                lc.choice("Stay vague: say you're here for work.",
                          lc.challenge("Say: no, I'm here for work.", "Non je suis ici pour le travail vacances oui",
                                       ["Non je suis ici pour le travail"], "SmallTalk"),
                          "c_size", flags=["CoverWork"]),
            ]),
    lc.node("c_size", T.PLAYER_CHALLENGE, "Très bien. Un grand ou un petit ?", "Very good. A large or a small?", "c_price",
            challenge_=lc.challenge("Ask for a small one, please.", "un petit s'il vous plaît grand deux",
                                    ["un petit s'il vous plaît", "petit s'il vous plaît"], "Ordering")),
    lc.node("c_price", T.NPC_LINE, "Ça fait deux euros cinquante.", "That'll be two euros fifty.", "c_thanks"),
    lc.node("c_thanks", T.PLAYER_CHALLENGE, "Voilà ! Bonne journée !", "Here you go! Have a good day!", "c_end",
            challenge_=lc.challenge("Thank her and wish her a good day too.",
                                    "Merci vous aussi bonne journée toi bonjour à",
                                    ["Merci vous aussi", "Merci bonne journée", "Merci à vous aussi"], "Politeness")),
    lc.node("c_end", T.END, "Au revoir, monsieur !", "Goodbye, sir! (Nicely done - she didn't suspect a thing.)"),
])

# --- Kiosk: code phrase. Blowing your cover here makes Marcel suspicious (mission branches) ---------------------
kiosk = lc.conversation(DATA, "DA_Conv_Kiosk", "Newspaper kiosk - meet the contact", paris, "v_greet", [
    lc.node("v_greet", T.PLAYER_CHALLENGE, "Bonjour ! Vous cherchez quelque chose ?", "Hello! Are you looking for something?", "v_dir",
            challenge_=lc.challenge("Say hello and ask where the station is.",
                                    "Bonjour où est la gare s'il vous plaît le musée qui",
                                    ["Bonjour où est la gare", "Bonjour où est la gare s'il vous plaît",
                                     "Bonjour s'il vous plaît où est la gare"], "Directions")),
    lc.node("v_dir", T.NPC_LINE, "La gare ? Allez tout droit, puis tournez à gauche.", "The station? Go straight ahead, then turn left.", "v_confirm"),
    lc.node("v_confirm", T.PLAYER_CHALLENGE, "C'est clair ?", "Is that clear?", "v_code",
            challenge_=lc.challenge("Repeat it back: straight ahead, then left.", "tout droit puis à gauche droite après",
                                    ["tout droit puis à gauche"], "Directions")),
    lc.node("v_code", T.PLAYER_CHALLENGE, "Vous aimez les journaux d'aujourd'hui ?",
            "Do you like today's newspapers? (That's the code phrase! Reply: 'I prefer yesterday's newspaper.')", "v_end",
            challenge_=lc.challenge("Give the code reply: 'I prefer yesterday's newspaper.'",
                                    "Je préfère le journal d'hier aime la demain",
                                    ["Je préfère le journal d'hier"], "Preferences", penalty=40.0)),
    lc.node("v_end", T.END, "Très bien... Le colis est au café, sous la table. Bonne chance.",
            "Very good... The package is at the café, under the table. Good luck.", outcome="GotLocation"),
], cover_blown_outcome="Suspicious")

kiosk_retry = lc.conversation(DATA, "DA_Conv_Kiosk_Retry", "Newspaper kiosk - second chance", paris, "r_start", [
    lc.node("r_start", T.PLAYER_CHALLENGE, "Encore vous ? Alors... le code ?", "You again? So... the code?", "r_end",
            challenge_=lc.challenge("Give the code reply: 'I prefer yesterday's newspaper.'",
                                    "Je préfère le journal d'hier aime la demain",
                                    ["Je préfère le journal d'hier"], "Preferences", penalty=40.0)),
    lc.node("r_end", T.END, "Bon. Le colis est au café, sous la table. Ne me revoyez plus.",
            "Fine. The package is at the café, under the table. Don't come back.", outcome="GotLocation"),
], cover_blown_outcome="Suspicious")

# --- Mission: First Contact (test mission; the full 'Opération Croissant' comes in M7) ---------------------------
first_contact = lc.mission(MISSIONS, "MA_Paris_FirstContact", "Paris_FirstContact", "Operation First Contact",
    "Handler: Welcome to Paris. Blend in at the café, then meet our contact Marcel at the newspaper kiosk. "
    "He'll tell you where the package is. Keep your French sharp.",
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
    ])

paris = lc.language_pack("/Game/LinguaSpy/Data/French", "DA_Lang_French_Paris", "Paris", "French", "Paris", "fr",
                         "Croissants, cafés and coded whispers.", "8FB8DE", 0,
                         city_map="/Game/LinguaSpy/Maps/Lvl_Paris",
                         missions=[first_contact], conversations=[cafe, kiosk, kiosk_retry])

# --- Placeholder packs for the other cities (maps + missions arrive in M8) ---------------------------------------
lc.language_pack("/Game/LinguaSpy/Data/Spanish", "DA_Lang_Spanish_Madrid", "Madrid", "Spanish", "Madrid", "es",
                 "Tapas, plazas and passwords at siesta time.", "F2A65A", 1)
lc.language_pack("/Game/LinguaSpy/Data/Japanese", "DA_Lang_Japanese_Tokyo", "Tokyo", "Japanese", "Tokyo", "ja",
                 "Konbini runs and whispered keigo.", "F4A7B9", 2, word_spacing=False, readings=True)
lc.language_pack("/Game/LinguaSpy/Data/Italian", "DA_Lang_Italian_Rome", "Rome", "Italian", "Rome", "it",
                 "Espresso at the bar, secrets by the fountain.", "E07A5F", 3)

print("Paris content built:", [a.get_name() for a in (cafe, kiosk, kiosk_retry, first_contact, paris)])
