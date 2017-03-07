# QBack
A simple bakcup utility.

![QBack_GUI](https://5c57bd3a-a-62cb3a1a-s-sites.googlegroups.com/site/gtronick/QBack_v1.1.0.PNG)

QBack lets you copy files from several directories to a folder. You can also specify only files, or only directories to copy. Works on Windows and Linux (Not tested on MAC OS).

1. HOW TO USE
-------------------------------

A. Copying one folder recursively:

  1. Click on the "Origin" button to choose the origin folder.
  2. Click on the "Target" button to choose the destination folder where the files will be copied.
  3. If you want to inspect the target folder before copy, click on "Open target".
  4. Click on the "Backup button" to start the copy.

B. Copying several folders recursively:

  1. Open your file explorer and copy the folder paths to the black text box, end each folder path with comma "," , for example: 

### In Linux: 

  /home/$USER/Downloads,
  /home/$USER/FilesToCopy,
  
### In Windows:
  
  D:\DocumentsToCopy\Folder,
  C:\Documents and settings\MyAppData\Game,

  2. Click on the "target" button to choose the destination folder.
  3. If you want to inspect the target folder before copy, click on "Open target".
  4. Click on the "Backup button" to start the copy.

C. Copying several files:

  1. Open your file explorer and copy the file paths to the black text box, end each file path with comma "," , for example:

### In Linux:

  /home/$USER/Downloads/Script.sh,
  /home/$USER/Documents/TextDocument.txt,

### In Windows:

  D:\DocumentsToCopy\Folder\WordDocument.docx,
  C:\Documents and settings\MyAppData\Game\Textures.mod,

  2. Click on the "target" button to choose the destination folder.
  3. If you want to inspect the target folder before copy, click on "Open target".
  4. Click on the "Backup button" to start the copy.

2. HOW TO BUILD FROM SOURCE
------------------------------------------------

QT5 must be installed. (qt5-base or download the QTCreator IDE from www.qt.io/download)

  1. qmake QBack.pro
  2. make all -j4
  3. make clean

Alternatively, you can use QTCreator 5.7+ to load the project, and compile it for your distribution and architecture.

3. TODO
------------------------------------------------

  1. Improve log information on errors.
  2. Play sound or show message on copy completion.
  3. Allow target folder creation when it does not exist.


