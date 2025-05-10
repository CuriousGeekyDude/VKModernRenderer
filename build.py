import subprocess
import gdown
import os

def run_batch_file(batch_file_path):
    """Run a .bat file."""
    try:
        subprocess.run(batch_file_path, check=True, shell=True)
        print(f"Successfully ran {batch_file_path}")
    except subprocess.CalledProcessError as e:
        print(f"Error running {batch_file_path}: {e}")

def download_sponza_assets():
    """Download files from a Google Drive folder into Assets\\Sponza."""
    output_dir = os.path.join("Assets", "Sponza")
    os.makedirs(output_dir, exist_ok=True)

    folder_url = "https://drive.google.com/drive/folders/1xiEzRC06PI-Ivg7JI6mbctKK9wTozsym"

    try:
        gdown.download_folder(url=folder_url, output=output_dir, quiet=False, use_cookies=False)
        print(f"Download completed. Files saved to: {output_dir}")
    except Exception as e:
        print(f"Error downloading folder: {e}")

if __name__ == "__main__":
    # Run the batch file first
    run_batch_file("InitializeRequiredFolders.bat")

    # Then download the Sponza assets
    download_sponza_assets()
