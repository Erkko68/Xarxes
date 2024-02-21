import sys # Used for user arguments

class Elements:
    def __init__(self, elements):
        # Split the string on ';'
        devices = elements.split(';')
        
        # Limit the size of the array to 10 devices
        if len(devices) > 10:
            print("[Warning] More than 10 devices detected, only the first 10 will be used.")
            devices = devices[:10] # Only take first 10 devices
        
        # Set devices array [0-9] as attribute
        setattr(self,'Device',devices)

class Config:
    def __init__(self, file_path):
        # Open file descriptor
        with open(file_path, 'r') as file:
            for line in file:
                # Split line based on '='
                key, value = line.split('=')
                # Split Devices
                if key == 'Elements':
                    value=Elements(value)
                # Set attribute to object
                setattr(self, key, value)

def main():
    # Check if the number of command-line arguments is correct
    if len(sys.argv) != 3 or sys.argv[1] != '-c':
        print("Usage: python script.py -c <config_file>")
        return

    # Get the file path
    file_path = sys.argv[2]

    # Create config object
    obj = Config(file_path)
    for key in obj.__dict__:
        print(f"{key}: {getattr(obj, key)}")

# Init call
if __name__ == "__main__":
    main()
