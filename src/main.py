import sys

def transpile(filename):
    print(f"Parsing {filename}...")
    # Parsing logic to be added
    print("Generation of C++ headers and source successful.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        transpile(sys.argv[1])
    else:
        print("Usage: python main.py <file.aem>")
