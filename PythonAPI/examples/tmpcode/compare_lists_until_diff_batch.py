
import csv
from pathlib import Path

def find_files(directory, search_string):
    path = Path(directory)
    return [str(file) for file in path.iterdir() 
            if file.is_file() and search_string in file.name]

def compare_csv_files(file1, file2, file2_skip_from=0, file2_skip_to=0):

    try:
        with open(file1, 'r', encoding='utf-8') as f1, open(file2, 'r', encoding='utf-8') as f2:
            reader1 = csv.reader(f1)
            reader2 = csv.reader(f2)
            
            row_count = 1+0
            for _ in range(row_count):
                next(reader1, None)
                next(reader2, None)
            
            for row_n0, (row1, row2) in enumerate(zip(reader1, reader2)):
                selected_cols = 5 if 'UWheeledVehicle4W' in file1 else 4
                data1 = row1[:selected_cols] if 'UWheeledVehicle4W' in file1 else row1[:selected_cols] + row1[-4:-3]
                data2 = row2[:selected_cols] if 'UWheeledVehicle4W' in file1 else row2[:selected_cols] + row2[-4:-3]

                while (data1[-1]!=data2[-1]):
                    row1 = next(reader1, None)
                    if row1 == None:
                        break
                    row_count += 1
                    data1 = row1[:selected_cols] if 'UWheeledVehicle4W' in file1 else row1[:selected_cols] + row1[-4:-3]

                if data1 != data2:
                    print('not identical:', file2)
                    break
                row_count += 1
            
            return row_count
    except Exception as e:
        print(f"error: {e}")
        return -1


if __name__ == '__main__':
    
    directory_path = "data/Determinism"
    search_term = "_VehicleDynamics"

    result_files = find_files(directory_path, search_term)

    file1 = result_files[0]
    print('\nref:', file1)
    for file2 in result_files[1:]:
        print('compare with:', file2)
        same_row_count = compare_csv_files(file1, file2, file2_skip_from=0, file2_skip_to=0)
        if same_row_count >= 0:
            print(f"the same frame no.: {0} - {same_row_count-2}\n")
        else:
            print("error")


