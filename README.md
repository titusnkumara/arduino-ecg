# ArduinoECG

This project contains code for an ECG peak detection and heart rate calculation algorithm to be used with an Arduino board. The code is written in C++ and consists of the following files:

- `main.cpp`
- `eventECG.cpp`
- `eventECG.h`
- `debugSettings.h`
- `Makefile`
- `Arduino-ino/Arduino-ino.ino`
- `Arduino-ino/eventECG.cpp`
- `Arduino-ino/Arduino-ino.ino`
## Windows Version

To compile and run the Windows version of the project, you will need a C++ compiler installed on your computer. The following instructions assume that you are using a Windows system.

### Getting Started

1. Download the project files and extract them to a folder on your computer.
2. Open a command prompt and navigate to the project directory using the following command:

```
cd path/to/project/folder
```

3. Compile the project using the included Makefile:

```
make ArduinoECG
```

If the makefile doesn't work for your system, you can compile the project manually by running the following commands:

```
g++ -c eventECG.cpp -o eventECG.o
g++ -c main.cpp -o main.o
g++ eventECG.o main.o -o ArduinoECG.exe
```

4. To run the program use .\ArduinoECG.exe \path\to\data\file
5. The output will be saved into a file called called output.txt

## Input

Input file is a text file with each sample of data is a data row. The input can be a floating point number or an integer. It can be either positive or negative.

## Output

The output file is a text file with following three columns
1. Sample number (x)
2. Peak location (y)
3. Calculated, instantanious heart rate (beats per minute)

## Usage

The Windows version of the ArduinoECG program is designed to read from a file while the Arduino version is taking the input from UART input. There is an example ECG data stream provided in data folder.
The sample data consists of three files,
- `100.txt` - This is the dataset 100 of MIT-BIH database converted into a txt file
- `100-ann.txt` - This is the annotated peaks in samples (x-axis)
- `100-type.txt` - This is the annotation type of each peak. For more information about beat types, please visit [database website](https://archive.physionet.org/physiobank/database/html/mitdbdir/mitdbdir.htm)

## Arduino Version

To compile and run the Arduino version of the project, you will need an Arduino board and the Arduino IDE installed on your computer. The following instructions assume that you are using a Windows system.

### Getting Started

1. Download the project files and extract them to a folder on your computer.
2. Open the `Arduino-ino.ino` file in the Arduino IDE.
3. Connect your Arduino board to your computer.
4. In the Arduino IDE, select the correct board type and port from the `Tools` menu.
5. Click the `Upload` button in the Arduino IDE to upload the code to the board.
6. Run the program and observe the output to verify that the ECG system is functioning as expected.



