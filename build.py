import os
import subprocess
import requests

# Step 1: Run the batch file to initialize folders
batch_file = "InitializeRequiredFolders.bat"
if os.path.exists(batch_file):
    subprocess.run(batch_file, shell=True)
    print("Folders initialized.")
else:
    print(f"Batch file {batch_file} not found. Skipping folder initialization.")

# Step 2: Download files from GitHub repo
api_url = "https://api.github.com/repos/KhronosGroup/glTF-Sample-Models/contents/2.0/Sponza"
download_folder = "Assets/Sponza"

# Ensure download directory exists
os.makedirs(download_folder, exist_ok=True)

def download_file(file_url, save_path):
    r = requests.get(file_url)
    r.raise_for_status()
    with open(save_path, 'wb') as f:
        f.write(r.content)
    print(f"Downloaded: {save_path}")

def process_directory(api_url, local_dir):
    os.makedirs(local_dir, exist_ok=True)
    response = requests.get(api_url)
    response.raise_for_status()
    files = response.json()

    for file in files:
        if file['type'] == 'file':
            download_file(file['download_url'], os.path.join(local_dir, file['name']))
        elif file['type'] == 'dir':
            process_directory(file['url'], os.path.join(local_dir, file['name']))

# Start downloading recursively
print("Downloading files from KhronosGroup/glTF-Sample-Models 2.0/Sponza...")
process_directory(api_url, download_folder)
print("\nAll files downloaded successfully to:", download_folder)
