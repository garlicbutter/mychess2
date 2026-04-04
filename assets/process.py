import os
from PIL import Image

def process_pieces():
    # Set up the directories
    source_dir = "."  # Current directory
    output_dir = "resized_pieces"
    
    # Create the output folder if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Target size for the LVGL widgets
    TARGET_SIZE = (30, 30)
    
    print("Scanning for chess pieces...")
    
    for filename in os.listdir(source_dir):
        # Filter for chess pieces only (ignores the chessboard or random files)
        # Based on your screenshot, pieces start with "Chess_" and have file extensions
        if filename.startswith("Chess_") and not filename.startswith("Chessboard"):
            
            filepath = os.path.join(source_dir, filename)
            
            try:
                # Pillow reads both PNG and WEBP automatically
                with Image.open(filepath) as img:
                    # Resize using high-quality LANCZOS filtering for smooth edges
                    resized_img = img.resize(TARGET_SIZE, Image.Resampling.LANCZOS)
                    
                    # Clean up the filename (e.g., 'Chess_bdt45.svg.webp' -> 'Chess_bdt45.png')
                    base_name = filename.split('.')[0]
                    new_filename = f"{base_name}.png"
                    new_filepath = os.path.join(output_dir, new_filename)
                    
                    # Force save as standard PNG
                    resized_img.save(new_filepath, format="PNG")
                    print(f"Processed: {filename.ljust(25)} -> {new_filename}")
                    
            except Exception as e:
                print(f"Skipped {filename}: {e}")

    print(f"\nDone! All 60x60 PNG pieces are saved in the '{output_dir}' folder.")

if __name__ == "__main__":
    process_pieces()