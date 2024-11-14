import shutil
from pathlib import Path

def clear_directory(directory):
    if directory.exists() and directory.is_dir():
        for item in directory.iterdir():
            if item.is_file():
                item.unlink()
            elif item.is_dir():
                shutil.rmtree(item)

def copy_files_and_libraries(libraries, files, dst_folder='./out'):
    dst_path = Path(dst_folder)
    clear_directory(dst_path)
    dst_path.mkdir(parents=True, exist_ok=True)

    # Copy .c and .h files from libraries
    for library in libraries:
        src_path = Path(library)
        if src_path.exists() and src_path.is_dir():
            for file in src_path.rglob('*'):
                if file.is_file() and file.suffix in ['.c', '.h']:
                    shutil.copy(file, dst_path / file.name)

    # Copy individual files
    for file in files:
        file_path = Path(file)
        if file_path.exists() and file_path.is_file():
            shutil.copy(file_path, dst_path / file_path.name)

libraries = [
    '../ParseTree',
    '../SymbolTable',
    '../utils',
]
files = [
    '../main.c',
    '../lexical.l',
    '../Makefile',
    '../syntax.y'
]

# Check existence of libraries and files
for path in libraries + files:
    if not Path(path).exists():
        print(f'{path} does not exist.')
        exit(1)

# Example usage
copy_files_and_libraries(libraries, files, './out1')
