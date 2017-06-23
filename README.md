# QBack
A simple, but powerful backup utility.

![QBack_GUI](https://sites.google.com/site/gtronick/QBack1.5.0.PNG)

QBack lets you copy files from several paths, to several targets. You can also specify only files, or only directories to copy. Works on Windows and Linux (Not tested on MAC OS).

# 1. HOW TO USE
-------------------------------

## A. Copying one folder recursively:

  1. Click on the "Origin" button to choose the origin folder.
  2. Click on the "Target" button to choose the destination folder where the files will be copied.
  3. If you want to inspect the target folder before copy, click on "Open target".
  4. Click on the "Backup button" to start the copy.

## B. Copying several folders recursively:

  1. Open your file explorer and copy the folder paths to the sources text area, end each folder path with comma ","; for example: 
  

  ### In Linux: 

    /home/$USER/Downloads,  
    /home/$USER/FilesToCopy,  
  
  ### In Windows:

    D:\DocumentsToCopy\Folder,  
    C:\Documents and settings\MyAppData\Game,  
    
  You can also drag and drop the folders into the sources text area. QBack will adapt the path adding the comma sign at the end.

  2. Click on the "target" button to choose the destination folder.
  3. If you want to inspect the target folder before copy, click on "Open target".
  4. Click on the "Backup button" to start the copy.

## C. Copying several files:

  1. Open your file explorer and copy the file paths to the sources text area, end each file path with comma ","; for example:
 
  ### In Linux:

    /home/$USER/Downloads/Script.sh,  
    /home/$USER/Documents/TextDocument.txt,  

  ### In Windows:

    D:\DocumentsToCopy\Folder\WordDocument.docx,  
    C:\Documents and settings\MyAppData\Game\Textures.mod,  

  You can also drag and drop the files into the sources text area. QBack will adapt the path adding the comma sign at the end.
  
  2. Click on the "Target" button to choose the destination folder.
  3. If you want to inspect the target folder before copy, click on "Open target".
  4. Click on the "Backup button" to start the copy.
  
## D. Copying several files and folders, to different targets:

  1. Open your file explorer and copy the file paths to the sources text area and append a comma at the end of each path, or drag and drop the files.
  2. If you want to copy a file to a certain target, use the '>' symbol, for example:
  
    /home/$USER/Downloads/Script.sh>/user/MyUser/media/USB_Storage,  
    /home/$USER/Documents/TextDocument.txt>/home/MyUser/BackupFolder,  
    /home/$USER/Downloads/QBack.sh,  
    /home/$USER/Documents/Text.txt,  
   
  This will cause QBack to copy the file Script.sh to /user/MyUser/media/USB_Storage, the file TextDocument.txt will be copied to /home/MyUser/BackupFolder, QBack.sh and Text.txt, will be copied to the default folder typed in the target line edit.
  
## E. Commenting paths:

  1. If you don't want to copy certain file in the list, just add the '#' sign to the start of the path to avoid its copy, for example:  

    #/home/$USER/Downloads/Script.sh>/user/MyUser/media/USB_Storage,  
    /home/$USER/Documents/TextDocument.txt>/home/MyUser/BackupFolder,  
    #/home/$USER/Downloads/QBack.sh,  
    /home/$USER/Documents/Text.txt,  
    
  This will avoid the copy of Script.sh, and QBack.sh.

# 2. HOW TO BUILD FROM SOURCE
------------------------------------------------

QT5 must be installed. (qt5-base or download the QTCreator IDE from www.qt.io/download)

    qmake QBack.pro
    make all -j4
    make clean

Alternatively, you can use QTCreator 5.8+ to load the project, and compile it for your distribution and architecture.


