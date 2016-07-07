import ftplib
from ftplib import FTP

pathToFile = "items.csv"
Output_Directory = "./"

try:
    ftp = FTP("88.86.121.67") #Endora surikata server
    ftp.login("mila95", "automobil123")
    file = open(pathToFile, "rb")
    ftp.cwd(Output_Directory)
    ftp.storbinary('STOR ' + pathToFile, file)
    ftp.quit()
    file.close()
    print("Database upload successful.")
except:
    print("An error occured while uploading the database file. Please \
check your Internet connection.")
