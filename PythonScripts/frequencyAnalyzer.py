import sys
import string

# Usage Error Checker
if (len(sys.argv) != 2):
    print("Usage: python [programName] [textFile]")
    exit(0)

# Print the arguments passed to the script
print("Reading file: " + sys.argv[1])

# Open the file
with open(sys.argv[1], "r") as file:

    # Prepare contents of the file for reading by removing punctuation, white space, and capitals
    contents = file.read().lower()
    contents = ''.join(char for char in contents if char.isalpha())

    # Create an array to hold the frequencies from a-z
    charCounts = [0 for i in range(26)]

    # For each character in contents, increment its corresponding count in the charCounts array
    for char in contents:
        charIndex = ord(char)-97
        charCounts[charIndex]+=1

    # Calculate the frequency percentage from the counts
    totalChars = sum(charCounts)
    charPercents = [0 for i in range(26)]
    for i in range(len(charCounts)):
        charPercents[i] = charCounts[i]/totalChars * 100
    charPercents = ["{:.2f}".format(number) for number in charPercents]

    # Print the results
    for i in range(26):
        #print(chr(i + 97), ": ", charPercents[i],"%")
        print(charPercents[i])
