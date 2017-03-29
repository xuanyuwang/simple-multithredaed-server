ext = ".txt"
content = """sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw
sadl;fjl;sajlfjljasldjfjasl;jfdaskjdhlksfsjvnafhptqniuw"""
for i in range(5):
    with open(str(i) + ext, 'w') as file:
        file.write(str(i) + "start: " + i * 5 * content + " end")
