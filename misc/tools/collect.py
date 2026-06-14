#!/usr/bin/env python3
"""
This script collects the entire source code and structure of the 'semon' project
into a single XML-like stream file (content.xml). The primary purpose of this 
dump is to provide full, well-structured context to Artificial Intelligence (AI) 
coding agents and language models.

The project structure and development are built according to the functional 
specifications defined in the 'docs/specification.md' file of this repository.
"""

import os
import xml.etree.ElementTree as ET
from xml.dom import minidom

# Determine the directory where the script itself is located
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Path to save the output file next to the script
OUTPUT_FILE = os.path.join(SCRIPT_DIR, "content.xml")

# Calculate the root directory of the "semon" project (two levels up from the script)
PROJECT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "../.."))

# Directories to ignore during the scan
IGNORE_DIRS = {'.git', '.vscode', 'build', 'cmake-build-debug', 'cmake-build-release'}
# Allowed project file extensions
VALID_EXTENSIONS = {'.cpp', '.hpp', '.h', '.c', '.txt', '.cmake', '.md', '.json', '.py'}

def is_text_file(file_path):
    """Checks if a file is a valid text file based on its extension."""
    _, ext = os.path.splitext(file_path)
    return ext.lower() in VALID_EXTENSIONS

def generate_project_xml():
    # Create the root XML element for the project
    root = ET.Element("project", name="semon", path=PROJECT_DIR)

    # Walk through the project directory tree
    for dirpath, dirnames, filenames in os.walk(PROJECT_DIR):
        # Exclude ignored directories in-place
        dirnames[:] = [d for d in dirnames if d not in IGNORE_DIRS]

        for filename in filenames:
            full_path = os.path.join(dirpath, filename)
            
            # Skip binary or unsupported files
            if not is_text_file(full_path):
                continue
                
            # Calculate the relative path from the project root
            rel_path = os.path.relpath(full_path, PROJECT_DIR)

            # If the current file is the content.xml file being generated, use a placeholder
            if filename == "content.xml":
                content = "[[[... Содержимое всего проекта в виде файла XML ...]]]"
            else:
                try:
                    with open(full_path, 'r', encoding='utf-8', errors='replace') as f:
                        content = f.read()
                except Exception as e:
                    content = f"[Error reading file: {str(e)}]"

            # Create an XML sub-element for the file
            file_node = ET.SubElement(root, "file", path=rel_path)
            file_node.text = content

    # Convert the XML tree to a pretty-printed string with indentation
    rough_string = ET.tostring(root, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ", encoding="utf-8").decode("utf-8")

if __name__ == "__main__":
    print(f"Collecting project files for AI context from: {PROJECT_DIR}...")
    
    # Generate the XML structure
    xml_output = generate_project_xml()
    
    # Write the output file into the script's local directory
    with open(OUTPUT_FILE, "w", encoding="utf-8") as xml_file:
        xml_file.write(xml_output)
        
    print(f"Success! Context file saved to: {OUTPUT_FILE}")
