def write_to_scv(date, data):
    with open('data.csv','r+') as file:
        file.write(f"{date};{data}\n")
        file.readlines()
