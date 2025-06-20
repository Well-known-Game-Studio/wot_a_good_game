import urllib.request
from io import BytesIO
from PIL import Image, ImageEnhance, ImageFilter
import pytesseract
from fuzzywuzzy import process
import json

# --- CONFIGURATION ---
MAP_FILENAME = "./westlands-national-borders-and-cities.png"
CITIES = [
    "Tar Valon", "The Two Rivers", "Shadar Logoth", "Whitebridge", "Fal Moran", "Tear",
    "Caemlyn", "Tanchico", "Ebou Dar", "Cairhien", "Illian", "Far Madding", "Maradon",
    "Jehannah", "Amador", "Bandar Eban", "Rhuidean", "Emond's Field", "Fal Dara",
    "Hinderstap", "Stedding Tsofu"
]
# IMG_SIZE = (1000, 1000)

# open the image
img = Image.open(MAP_FILENAME).convert("RGB")
# img = img.resize(IMG_SIZE, Image.LANCZOS)

# Preprocess for OCR
# gray = img.convert("L").filter(ImageFilter.SHARPEN)
# gray = ImageEnhance.Contrast(gray).enhance(2.5)
# bw = gray.point(lambda x: 0 if x < 160 else 255, '1')
bw = img

# --- OCR AND GROUPING ---
data = pytesseract.image_to_data(bw, output_type=pytesseract.Output.DICT, config='--psm 11')
words = []
for i, text in enumerate(data['text']):
    if text.strip():
        words.append({
            'text': text.strip(),
            'left': data['left'][i],
            'top': data['top'][i],
            'width': data['width'][i],
            'height': data['height'][i]
        })

# Group words by proximity
grouped = []
used = set()
for i, w1 in enumerate(words):
    if i in used:
        continue
    group = [w1]
    for j, w2 in enumerate(words):
        if i != j and j not in used:
            if abs(w1['left'] - w2['left']) < 40 and abs(w1['top'] - w2['top']) < 60:
                group.append(w2)
                used.add(j)
    grouped.append(group)

# Fuzzy match grouped words to city list
matches = []
for group in grouped:
    group_text = ' '.join([w['text'] for w in sorted(group, key=lambda x: x['top'])])
    match, score = process.extractOne(group_text, CITIES)
    if score > 80:
        avg_x = int(sum(w['left'] + w['width']//2 for w in group) / len(group))
        avg_y = int(sum(w['top'] + w['height']//2 for w in group) / len(group))
        matches.append({
            'city': match,
            'ocr_text': group_text,
            'score': score,
            'pixel': (avg_x, avg_y),
            'offset': (0, 0)
        })

matches = sorted(matches, key=lambda m: m['score'], reverse=True)
print("Detected cities and their pixel locations:")
for m in matches:
    print(m)

# Save matches for Blender
with open("city_matches.json", "w") as f:
    json.dump(matches, f)
img.save("processed_map.png")
