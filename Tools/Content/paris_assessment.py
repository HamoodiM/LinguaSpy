"""French assessment forms FR_A / FR_B for the pilot study (docs/Pilot_Study_Protocol.md).
Parallel forms: same skills and item types in the same order, different phrases. Listening, reading and gap items
recombine the taught words in new sentences (transfer). A few assembly items (A13-A15, B13, B15) are core phrases the
game teaches almost word for word; analysis can report them separately (item ids are in the log).
Each form: 5 listen-and-choose, 4 read-and-choose, 3 gap-fill, 3 sentence assembly = 15 items.
Run inside the editor after paris_content.py, then voice_build.py (adds audio to the listening items):
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/paris_assessment.py", encoding="utf-8").read())
"""
import importlib, sys
sys.path.insert(0, r"C:/UnrealEngineProjects/Personal/Tools/Content")
import lingua_content as lc
importlib.reload(lc)
import unreal

EAL = unreal.EditorAssetLibrary
A = unreal.AssessmentItemType
FOLDER = "/Game/LinguaSpy/Data/French/Assessment"
paris = EAL.load_asset("/Game/LinguaSpy/Data/French/DA_Lang_French_Paris")


def choose(item_id, kind, prompt, target, options, correct, skill):
    i = unreal.AssessmentItem()
    i.set_editor_property("item_id", item_id)
    i.set_editor_property("type", kind)
    i.set_editor_property("prompt", prompt)
    i.set_editor_property("target_text", target)
    i.set_editor_property("options", options)
    i.set_editor_property("correct_index", correct)
    i.set_editor_property("skill_tag", skill)
    return i


def listen(item_id, target, options, correct, skill):
    return choose(item_id, A.LISTEN_CHOOSE, "Listen. What does it mean?", target, options, correct, skill)


def read(item_id, target, options, correct, skill):
    return choose(item_id, A.MEANING_CHOOSE, "What does this mean?", target, options, correct, skill)


def gap(item_id, prompt, answer, gaps, skill):
    i = unreal.AssessmentItem()
    i.set_editor_property("item_id", item_id)
    i.set_editor_property("type", A.GAP_FILL)
    i.set_editor_property("prompt", prompt)
    c = lc.challenge(prompt, answer, [answer], skill, gaps=gaps)
    i.set_editor_property("challenge", c)
    i.set_editor_property("skill_tag", skill)
    return i


def assemble(item_id, prompt, tiles, answer, skill):
    i = unreal.AssessmentItem()
    i.set_editor_property("item_id", item_id)
    i.set_editor_property("type", A.ASSEMBLE)
    i.set_editor_property("prompt", prompt)
    i.set_editor_property("challenge", lc.challenge(prompt, tiles, [answer], skill))
    i.set_editor_property("skill_tag", skill)
    return i


def form(name, form_id, title, items):
    f = lc.get_or_create(FOLDER, name, unreal.AssessmentAsset)
    f.set_editor_property("form_id", form_id)
    f.set_editor_property("language", paris)
    f.set_editor_property("title", title)
    f.set_editor_property("items", items)
    problems = f.validate()
    if problems:
        unreal.log_warning(f"{name}: " + "; ".join(str(p) for p in problems))
    EAL.save_loaded_asset(f, False)
    return f


form_a = form("DA_Assess_French_A", "FR_A", "French field test A", [
    listen("A01", "Merci beaucoup", ["Thank you very much", "Good morning", "See you soon"], 0, "Politeness"),
    listen("A02", "Où est le métro ?", ["What time is it?", "Where is the metro?", "How much is the metro?"], 1, "Directions"),
    listen("A03", "Je voudrais un thé", ["I have a tea", "I'm making tea", "I would like a tea"], 2, "Ordering"),
    listen("A04", "Ça fait trois euros", ["That's three euros", "That's two euros", "It's three o'clock"], 0, "Numbers"),
    listen("A05", "Tournez à droite", ["Turn left", "Turn right", "Go straight on"], 1, "Directions"),
    read("A06", "Bonne journée !", ["Good evening!", "Good luck!", "Have a good day!"], 2, "Politeness"),
    read("A07", "Je suis ici pour le travail", ["I'm here for work", "I'm on holiday", "I'm looking for work"], 0, "SmallTalk"),
    read("A08", "Il est midi", ["It's midnight", "It's noon", "It's ten o'clock"], 1, "Time"),
    read("A09", "Une seule, s'il vous plaît", ["A small one, please", "Cash only, please", "Just one, please"], 2, "Numbers"),
    gap("A10", "Say: 'Hello, I would like a croissant.'", "Bonjour je voudrais un croissant", [2], "Ordering"),
    gap("A11", "Ask: 'Where is the bakery?'", "Où est la boulangerie", [0], "Directions"),
    gap("A12", "Ask: 'Can I pay by card?'", "Je peux payer par carte", [4], "Shopping"),
    assemble("A13", "Say: 'A coffee, please.'", "Un café s'il vous plaît thé merci", "Un café s'il vous plaît", "Ordering"),
    assemble("A14", "Ask: 'What time is it?'", "Quelle heure est-il où quel", "Quelle heure est-il", "Time"),
    assemble("A15", "Say: 'Yes, I'm on holiday.'", "Oui je suis en vacances non travail", "Oui je suis en vacances", "SmallTalk"),
])

form_b = form("DA_Assess_French_B", "FR_B", "French field test B", [
    listen("B01", "Merci, au revoir", ["Hello, thanks", "Thanks, goodbye", "See you tomorrow"], 1, "Politeness"),
    listen("B02", "Où est le pont ?", ["Where is the bridge?", "Where is the station?", "What is the bridge?"], 0, "Directions"),
    listen("B03", "Je voudrais un croissant", ["I'm eating a croissant", "I have a croissant", "I would like a croissant"], 2, "Ordering"),
    listen("B04", "Ça fait un euro vingt", ["That's one euro ten", "That's one euro twenty", "That's two euros twenty"], 1, "Numbers"),
    listen("B05", "Allez tout droit", ["Go straight on", "Turn left", "Go back"], 0, "Directions"),
    read("B06", "Bonne soirée !", ["Have a good day!", "Have a good evening!", "Good luck!"], 1, "Politeness"),
    read("B07", "Je suis en vacances", ["I'm here for work", "I'm on the train", "I'm on holiday"], 2, "SmallTalk"),
    read("B08", "Il est trois heures", ["It's three o'clock", "It's three euros", "It's two o'clock"], 0, "Time"),
    read("B09", "Un petit, s'il vous plaît", ["A large one, please", "A small one, please", "Just one, please"], 1, "Ordering"),
    gap("B10", "Say: 'Hello, I would like a baguette.'", "Bonjour je voudrais une baguette", [2], "Ordering"),
    gap("B11", "Ask: 'Where is the museum?'", "Où est le musée", [0], "Directions"),
    gap("B12", "Say: 'Here's two euros.'", "Voilà deux euros", [2], "Numbers"),
    assemble("B13", "Say: 'A ticket, please.'", "Un ticket s'il vous plaît billet merci", "Un ticket s'il vous plaît", "Ordering"),
    assemble("B14", "Say: 'I prefer tea.'", "Je préfère le thé aime café", "Je préfère le thé", "Preferences"),
    assemble("B15", "Say: 'No, I'm here for work.'", "Non je suis ici pour le travail oui vacances", "Non je suis ici pour le travail", "SmallTalk"),
])

print("Assessment forms built:", form_a.get_name(), form_b.get_name())
