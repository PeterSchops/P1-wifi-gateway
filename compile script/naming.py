Import("env")
import os
import re

def extract_define_value(file_path, define_name):
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            # Search for the pattern #define DEFINE_NAME value
            pattern = rf'#define\s+{define_name}\s+[\""]?([^\""\n]+)[\""]?'
            match = re.search(pattern, content)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Error reading file: {e}")
    return None

# Path to your source file containing the DEFINE
source_file = "src/GlobalVar.h"  # Adjust the path according to your structure

Src_Name = "HOSTNAME"
Src_Version = "VERSION"

# Value extraction
Name = extract_define_value(source_file, Src_Name)
Language = env.GetProjectOption("custom_language")
Version = extract_define_value(source_file, Src_Version)

# Changing the program name
program_name = f"{Name}-{Language}-{Version}"
env.Replace(PROGNAME=program_name)
print(f"Program name set to: {program_name}")