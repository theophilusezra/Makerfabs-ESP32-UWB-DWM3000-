import sys
import serial

from PyQt6.QtCore import Qt, QTimer, QTime
from PyQt6.QtGui import QPen, QTransform, QColor
from PyQt6.QtWidgets import QApplication, QMainWindow, QGraphicsScene, QGraphicsEllipseItem, QGraphicsLineItem

from MainDW3000IPSv2 import Ui_MainDW3000IPS
from globalVariable import (ScreenLength_globalVar, ScreenWidth_globalVar, max_width, max_height, x1_globalVar, y1_globalVar, x2_globalVar, y2_globalVar, x3_globalVar, y3_globalVar, COM_globalVar, Baudrate_globalVar, t1a1_globalVar, t1a2_globalVar, t1a3_globalVar, t2a1_globalVar, t2a2_globalVar, t2a3_globalVar, t3a1_globalVar, t3a2_globalVar, t3a3_globalVar)

class IndoorPositioningApp(QMainWindow):
    def __init__(self):
        super().__init__()

        self.ui = Ui_MainDW3000IPS()
        self.ui.setupUi(self)

        self.ui.lineEdit_x1AP.textChanged.connect(self.enableSaveAP)
        self.ui.lineEdit_y1AP.textChanged.connect(self.enableSaveAP)
        self.ui.checkBox_1AP.stateChanged.connect(self.enableSaveAP)

        self.ui.lineEdit_x2AP.textChanged.connect(self.enableSaveAP)
        self.ui.lineEdit_y2AP.textChanged.connect(self.enableSaveAP)
        self.ui.checkBox_2AP.stateChanged.connect(self.enableSaveAP)

        self.ui.lineEdit_x3AP.textChanged.connect(self.enableSaveAP)
        self.ui.lineEdit_y3AP.textChanged.connect(self.enableSaveAP)
        self.ui.checkBox_3AP.stateChanged.connect(self.enableSaveAP)

        self.ui.pushButton_SAVE_CS.clicked.connect(self.SaveValueAP)
        self.ui.pushButton_SAVE_CS.clicked.connect(self.enableSetIPS)
        self.ui.pushButton_SAVE_CS.clicked.connect(self.enableSerial)

        self.ui.lineEdit_lengthResult_AC.textChanged.connect(self.enableSaveAP)
        self.ui.lineEdit_widthResult_AC.textChanged.connect(self.enableSaveAP)

        self.ui.comboBox_COM_CS.currentIndexChanged.connect(self.enableSaveAP)
        self.ui.comboBox_Baudrate_CS.currentIndexChanged.connect(self.enableSaveAP)

        self.ui.pushButton_SET_CS.clicked.connect(self.setSetAP)

        self.ui.pushButton_RESET_CS.clicked.connect(self.resetValuesAP)

        self.scene = QGraphicsScene()
        self.ui.graphicsView_IPS_MW.setScene(self.scene)

        # badge timer
        self.timer_elapsed = QTimer(self)
        self.timer_elapsed.setInterval(1000)
        self.timer_elapsed.timeout.connect(self.updateTimer)
        self.elapsed_time = QTime(0, 0)

        # timer serial
        self.timer_serial = QTimer(self)
        self.timer_serial.timeout.connect(self.update_values)

        # timer update view
        self.timer_tag = QTimer(self)
        self.timer_tag.timeout.connect(self.updateTagPosition)
        self.tag = None
        self.tag_label = None

    def updateTimer(self):
        self.elapsed_time = self.elapsed_time.addSecs(1)
        self.ui.label_ConnectedTimeDisplay_MW.setText(self.elapsed_time.toString("hh : mm : ss"))

    def enableSaveAP(self):
        global ScreenLength_globalVar, ScreenWidth_globalVar, gridValue, x1_globalVar, y1_globalVar, x2_globalVar, y2_globalVar, x3_globalVar, y3_globalVar, COM_globalVar, Baudrate_globalVar, AreaCover

        x1_globalVar = self.ui.lineEdit_x1AP.text()
        y1_globalVar = self.ui.lineEdit_y1AP.text()
        x2_globalVar = self.ui.lineEdit_x2AP.text()
        y2_globalVar = self.ui.lineEdit_y2AP.text()
        x3_globalVar = self.ui.lineEdit_x3AP.text()
        y3_globalVar = self.ui.lineEdit_y3AP.text()

        ScreenLength_globalVar = self.ui.lineEdit_lengthResult_AC.text()
        ScreenWidth_globalVar = self.ui.lineEdit_widthResult_AC.text()
        gridValue = self.ui.lineEdit_gridResult_AC.text()

        try:
            float(ScreenLength_globalVar)
            float(ScreenWidth_globalVar)
            float(gridValue)
            AreaCover = True
        except ValueError:
            AreaCover = False

        COM_globalVar = self.ui.comboBox_COM_CS.currentText()
        selectedCOM_globalVar = self.ui.comboBox_COM_CS.currentIndex()

        Baudrate_globalVar = self.ui.comboBox_Baudrate_CS.currentText()
        selectedBaudrate_globalVar = self.ui.comboBox_Baudrate_CS.currentIndex()

        try:
            float(x1_globalVar)
            float(y1_globalVar)
            self.ui.checkBox_1AP.setChecked(True)
        except ValueError:
            self.ui.checkBox_1AP.setChecked(False)

        try:
            float(x2_globalVar)
            float(y2_globalVar)
            self.ui.checkBox_2AP.setChecked(True)
        except ValueError:
            self.ui.checkBox_2AP.setChecked(False)

        try:
            float(x3_globalVar)
            float(y3_globalVar)
            self.ui.checkBox_3AP.setChecked(True)
        except ValueError:
            self.ui.checkBox_3AP.setChecked(False)

        checked_1AP = self.ui.checkBox_1AP.isChecked()
        checked_2AP = self.ui.checkBox_2AP.isChecked()
        checked_3AP = self.ui.checkBox_3AP.isChecked()

        if checked_1AP and checked_2AP and checked_3AP and AreaCover and selectedCOM_globalVar > 0 and selectedBaudrate_globalVar > 0:
            self.ui.pushButton_SAVE_CS.setEnabled(True)
        else:
            self.ui.pushButton_SAVE_CS.setEnabled(False)

    def SaveValueAP(self):
        global ScreenLength_globalVar, ScreenWidth_globalVar, gridValue, x1_globalVar, y1_globalVar, x2_globalVar, y2_globalVar, x3_globalVar, y3_globalVar, COM_globalVar, Baudrate_globalVar, AreaCover, EnableSET

        self.ui.label_x1_MW.setText(f"{x1_globalVar}")
        self.ui.label_y1_MW.setText(f"{y1_globalVar}")
        self.ui.label_x2_MW.setText(f"{x2_globalVar}")
        self.ui.label_y2_MW.setText(f"{y2_globalVar}")
        self.ui.label_x3_MW.setText(f"{x3_globalVar}")
        self.ui.label_y3_MW.setText(f"{y3_globalVar}")

        print(f"1st Anchor: {x1_globalVar},{y1_globalVar}")
        print(f"2nd Anchor: {x2_globalVar},{y2_globalVar}")
        print(f"3rd Anchor: {x3_globalVar},{y3_globalVar}")

        if AreaCover:
            print("Area Cover adalah True")
            print(f"Area Display: {ScreenLength_globalVar} x {ScreenWidth_globalVar} with {gridValue}")
        else:
            print("Area Cover adalah False")

        print("Selected COM Value:", COM_globalVar)
        print("Selected Baudrate Value:", Baudrate_globalVar)

        if float(gridValue) < 1:
            screen_length = int(ScreenLength_globalVar) * 1000
            screen_width = int(ScreenWidth_globalVar) * 1000
        else:
            screen_length = int(ScreenLength_globalVar) * 100
            screen_width = int(ScreenWidth_globalVar) * 100

        # if int(ScreenWidth_globalVar) < 10:
        #     screen_width = int(ScreenWidth_globalVar) * 200
        # else:
        #     screen_width = int(ScreenWidth_globalVar) * 100

        self.scene.setSceneRect(0, 0, screen_length, screen_width)

        self.ui.horizontalScrollBar_IPS_MW.setRange(0, int(screen_length) - int(max_width))
        self.ui.verticalScrollBar_IPS_MW.setRange(0, int(screen_width) - int(max_height))

        # Connect scroll bars to view
        self.ui.horizontalScrollBar_IPS_MW.valueChanged.connect(self.updateScrollBarView)
        self.ui.verticalScrollBar_IPS_MW.valueChanged.connect(self.updateScrollBarView)
        self.ui.horizontalSlider_ZoomInOut.valueChanged.connect(self.zoomInOut)

        # Tambahkan grid
        self.addGrid()
        self.addCentre(screen_length/2,screen_width/2, "(0,0)")

        if float(gridValue) < 1:
            x1 = abs((float(screen_length)/2) + (float(x1_globalVar) * 1000))
            y1 = abs((float(screen_width)/2) - (float(y1_globalVar) * 1000))
            x2 = abs((float(screen_length)/2) + (float(x2_globalVar) * 1000))
            y2 = abs((float(screen_width)/2) - (float(y2_globalVar) * 1000))
            x3 = abs((float(screen_length)/2) + (float(x3_globalVar) * 1000))
            y3 = abs((float(screen_width)/2) - (float(y3_globalVar) * 1000))
        else:
            x1 = abs((float(screen_length)/2) + (float(x1_globalVar) * 100))
            y1 = abs((float(screen_width)/2) - (float(y1_globalVar) * 100))
            x2 = abs((float(screen_length)/2) + (float(x2_globalVar) * 100))
            y2 = abs((float(screen_width)/2) - (float(y2_globalVar) * 100))
            x3 = abs((float(screen_length)/2) + (float(x3_globalVar) * 100))
            y3 = abs((float(screen_width)/2) - (float(y3_globalVar) * 100))

        self.addAnchor1(x1, y1, f"1st Anchor ({x1_globalVar},{y1_globalVar})")
        self.addAnchor2(x2, y2, f"2nd Anchor ({x2_globalVar},{y2_globalVar})")
        self.addAnchor3(x3, y3, f"3rd Anchor ({x3_globalVar},{y3_globalVar})")

        self.ui.pushButton_SAVE_CS.setEnabled(False)
        EnableSET = True

    def enableSerial(self):
        global reading_serial
        reading_serial = True
        self.timer_serial.start(500)
        print("Reading Serial is enable. Please SET to run the trilateration!")

    def update_values(self):
        global reading_serial, t1a1_globalVar, t1a2_globalVar, t1a3_globalVar, COM_globalVar, Baudrate_globalVar, is_floatT1

        if reading_serial:
            try:
                # Open the serial port and read a line
                with serial.Serial(COM_globalVar, Baudrate_globalVar, timeout=1) as ser:
                    line = ser.readline().decode().strip()

                # Split the line into three values
                values = [float(x) for x in line.split(',')]

                # Update the global variables
                t1a1_globalVar, t1a2_globalVar, t1a3_globalVar = values

                is_floatT1 = True
                self.ui.checkBox_RangingTag1_MW.setChecked(True)


            except Exception as e:
                pass

        # Update QLabel texts with the global variables
        self.ui.label_valuet1a1_MW.setText(f'{t1a1_globalVar}')
        self.ui.label_valuet1a2_MW.setText(f'{t1a2_globalVar}')
        self.ui.label_valuet1a3_MW.setText(f'{t1a3_globalVar}')

    def enableSetIPS(self):
        global EnableSET

        SAVE = self.ui.pushButton_SAVE_CS.isEnabled()
        if EnableSET and not SAVE :
            self.ui.pushButton_SET_CS.setEnabled(True)
            print("SET Button is enable")
        else:
            self.ui.pushButton_SET_CS.setEnabled(False)

    def setSetAP(self):
        global x1_globalVar, y1_globalVar, x2_globalVar, y2_globalVar, x3_globalVar, y3_globalVar, t1a2_globalVar, t1a3_globalVar, t2a1_globalVar, t2a2_globalVar, t2a3_globalVar, t3a1_globalVar, t3a2_globalVar, t3a3_globalVar, is_floatT1, is_floatT2, is_floatT3

        if is_floatT1 or is_floatT2 or is_floatT3:
            self.timer_elapsed.start()
            self.ui.label_StatusConnectionResult.setText("CONNECTED!")
            self.timer_tag.start(1000)
        else:
            self.timer_tag.stop()
            self.timer_elapsed.stop()
            self.elapsed_time = QTime(0, 0)
            self.ui.label_ConnectedTimeDisplay_MW.setText("00 : 00 : 00")
            self.ui.label_StatusConnectionResult.setText("NOT CONNECTED!")

        self.ui.pushButton_SET_CS.setEnabled(False)
        self.ui.pushButton_RESET_CS.setEnabled(True)

    def resetValuesAP(self):
        global  x1_globalVar, y1_globalVar, x2_globalVar, y2_globalVar, x3_globalVar, y3_globalVar, COM_globalVar, Baudrate_globalVar, ScreenLength_globalVar, ScreenWidth_globalVar, gridValue, t1a1_globalVar, t1a2_globalVar, t1a3_globalVar, t2a1_globalVar, t2a2_globalVar, t2a3_globalVar, t3a1_globalVar, t3a2_globalVar, t3a3_globalVar, EnableSET, reading_serial, is_floatT1, is_floatT2, is_floatT3

        self.scene.clear()

        self.timer_tag.stop()
        self.tag = None
        self.tag_label = None

        self.timer_serial.stop()
        self.timer_elapsed.stop()
        self.elapsed_time = QTime(0, 0)
        self.ui.label_ConnectedTimeDisplay_MW.setText("00 : 00 : 00")
        self.ui.label_StatusConnectionResult.setText("NOT CONNECTED!")

        self.ui.lineEdit_x1AP.clear()
        self.ui.lineEdit_y1AP.clear()

        self.ui.lineEdit_x2AP.clear()
        self.ui.lineEdit_y2AP.clear()

        self.ui.lineEdit_x3AP.clear()
        self.ui.lineEdit_y3AP.clear()

        self.ui.label_x1_MW.setText(f"x1")
        self.ui.label_y1_MW.setText(f"y1")

        self.ui.label_x2_MW.setText(f"x2")
        self.ui.label_y2_MW.setText(f"y2")

        self.ui.label_x3_MW.setText(f"x3")
        self.ui.label_y3_MW.setText(f"y3")

        self.ui.checkBox_1AP.setChecked(False)
        self.ui.checkBox_2AP.setChecked(False)
        self.ui.checkBox_3AP.setChecked(False)

        self.ui.comboBox_COM_CS.setCurrentIndex(0)
        self.ui.comboBox_Baudrate_CS.setCurrentIndex(0)

        self.ui.lineEdit_lengthResult_AC.clear()
        self.ui.lineEdit_widthResult_AC.clear()
        self.ui.lineEdit_gridResult_AC.clear()

        x1_globalVar = ""
        y1_globalVar = ""
        x2_globalVar = ""
        y2_globalVar = ""
        x3_globalVar = ""
        y3_globalVar = ""

        COM_globalVar = ""
        Baudrate_globalVar = ""

        ScreenLength_globalVar = ""
        ScreenWidth_globalVar = ""
        gridValue = ""

        t1a1_globalVar = ""
        t1a2_globalVar = ""
        t1a3_globalVar = ""

        self.ui.checkBox_RangingTag1_MW.setChecked(False)
        self.ui.label_valuet1a1_MW.setText(f"t1a1")
        self.ui.label_valuet1a2_MW.setText(f"t1a2")
        self.ui.label_valuet1a3_MW.setText(f"t1a3")

        t2a1_globalVar = ""
        t2a2_globalVar = ""
        t2a3_globalVar = ""

        self.ui.checkBox_RangingTag2_MW.setChecked(False)
        self.ui.label_valuet2a1_MW.setText(f"t2a1")
        self.ui.label_valuet2a2_MW.setText(f"t2a2")
        self.ui.label_valuet2a3_MW.setText(f"t2a3")

        t3a1_globalVar = ""
        t3a2_globalVar = ""
        t3a3_globalVar = ""

        self.ui.checkBox_RangingTag3_MW.setChecked(False)
        self.ui.label_valuet3a1_MW.setText(f"t3a1")
        self.ui.label_valuet3a2_MW.setText(f"t3a2")
        self.ui.label_valuet3a3_MW.setText(f"t3a3")

        self.ui.pushButton_RESET_CS.setEnabled(False)

        EnableSET = False
        reading_serial = False

        is_floatT1 = False
        is_floatT2 = False
        is_floatT3 = False


    # function for graphicsView
# # # # # # # # #  # # # #  START HERE # # # # # # # # #  # # # #

    # function for show Grid and Anchors
    def updateScrollBarView(self):
        global max_width, max_height
        x_bar = self.ui.horizontalScrollBar_IPS_MW.value()
        y_bar = self.ui.verticalScrollBar_IPS_MW.value()

        self.ui.graphicsView_IPS_MW.setSceneRect(x_bar, y_bar, max_width, max_height)

    def zoomInOut(self):
        zoom_factor = self.ui.horizontalSlider_ZoomInOut.value() / 100.0
        self.ui.graphicsView_IPS_MW.setTransform(QTransform().scale(zoom_factor, zoom_factor))

    def addGrid(self):
        global gridValue

        # Hitung panjang dan lebar layar atau scene

        # Hitung ukuran grid
        if float(gridValue) < 1:
            grid_size = abs(float(gridValue) * 1000)
            screen_length = int(self.scene.width())*100
            screen_width = int(self.scene.height())*100
        else:
            grid_size = abs(float(gridValue) * 100)
            screen_length = int(self.scene.width())
            screen_width = int(self.scene.height())

        pen = QPen(QColor(Qt.GlobalColor.lightGray))

        # Tambahkan garis grid untuk sumbu x
        for x in range(0, screen_length, int(grid_size)):
            line = QGraphicsLineItem(x, 0, x, screen_width)
            line.setPen(pen)
            self.scene.addItem(line)

        # Tambahkan garis grid untuk sumbu y
        for y in range(0, screen_width, int(grid_size)):
            line = QGraphicsLineItem(0, y, screen_length, y)
            line.setPen(pen)
            self.scene.addItem(line)

    def addAnchor1(self, x, y, label):
        if float(gridValue) < 1:
            anchor_radius = 15
        else:
            anchor_radius = 8

        anchor = QGraphicsEllipseItem(x - anchor_radius, y - anchor_radius, 2 * anchor_radius, 2 * anchor_radius)
        anchor.setBrush(Qt.GlobalColor.red)
        self.scene.addItem(anchor)

        anchor_label = self.scene.addText(label)
        anchor_label.setPos(x - 35, y + 10)
        anchor_label.setDefaultTextColor(Qt.GlobalColor.black)
        self.scene.addItem(anchor_label)

    def addAnchor2(self, x, y, label):
        if float(gridValue) < 1:
            anchor_radius = 15
        else:
            anchor_radius = 8

        anchor = QGraphicsEllipseItem(x - anchor_radius, y - anchor_radius, 2 * anchor_radius, 2 * anchor_radius)
        anchor.setBrush(Qt.GlobalColor.blue)
        self.scene.addItem(anchor)

        anchor_label = self.scene.addText(label)
        anchor_label.setPos(x - 35, y + 10)
        anchor_label.setDefaultTextColor(Qt.GlobalColor.black)
        self.scene.addItem(anchor_label)

    def addAnchor3(self, x, y, label):
        if float(gridValue) < 1:
            anchor_radius = 15
        else:
            anchor_radius = 8

        anchor = QGraphicsEllipseItem(x - anchor_radius, y - anchor_radius, 2 * anchor_radius, 2 * anchor_radius)
        anchor.setBrush(Qt.GlobalColor.darkYellow)
        self.scene.addItem(anchor)

        anchor_label = self.scene.addText(label)
        anchor_label.setPos(x - 35, y + 10)
        anchor_label.setDefaultTextColor(Qt.GlobalColor.black)
        self.scene.addItem(anchor_label)

    def addCentre(self, x, y, label):
        global gridValue
        if float(gridValue) < 1:
            anchor_radius = 15
        else:
            anchor_radius = 8

        centre = QGraphicsEllipseItem(x - anchor_radius, y - anchor_radius, 2 * anchor_radius, 2 * anchor_radius)
        centre.setBrush(Qt.GlobalColor.black)
        self.scene.addItem(centre)

        centre_label = self.scene.addText(label)
        centre_label.setPos(x - 15, y + 10)
        centre_label.setDefaultTextColor(Qt.GlobalColor.black)
        self.scene.addItem(centre_label)

    def addTag(self, x, y, label):
        global gridValue
        if float(gridValue) < 1:
            anchor_radius = 15
        else:
            anchor_radius = 8

        if self.tag is not None and self.tag_label is not None:
            self.scene.removeItem(self.tag)
            self.scene.removeItem(self.tag_label)

        tag = QGraphicsEllipseItem(x - anchor_radius, y - anchor_radius, 2 * anchor_radius, 2 * anchor_radius)
        tag.setBrush(Qt.GlobalColor.green)

        tag_label = self.scene.addText(label)
        tag_label.setPos(x - 35, y + 10)
        tag_label.setDefaultTextColor(Qt.GlobalColor.darkGreen)

        self.tag = tag
        self.tag_label = tag_label

        self.scene.addItem(tag)
        self.scene.addItem(self.tag_label)

    def updateTagPosition(self):
        global x1_globalVar, y1_globalVar, x2_globalVar, y2_globalVar, x3_globalVar, y3_globalVar, t1a1_globalVar, t1a2_globalVar, t1a3_globalVar, ScreenLength_globalVar, ScreenWidth_globalVar, gridValue

        anchor1 = (float(x1_globalVar), float(y1_globalVar))
        anchor2 = (float(x2_globalVar), float(y2_globalVar))
        anchor3 = (float(x3_globalVar), float(y3_globalVar))

        # Mendapatkan jarak dari tag ke masing-masing anchor (contoh jarak)
        distance1 = abs(float(t1a1_globalVar))
        distance2 = abs(float(t1a2_globalVar))
        distance3 = abs(float(t1a3_globalVar))

        # Melakukan trilaterasi untuk mendapatkan posisi tag
        tag_x, tag_y = self.trilateration(anchor1, anchor2, anchor3, distance1, distance2, distance3)

        if float(gridValue) < 1:
            screen_length = int(ScreenLength_globalVar) * 1000
            screen_width = int(ScreenWidth_globalVar) * 1000
        else:
            screen_length = int(ScreenLength_globalVar) * 100
            screen_width = int(ScreenWidth_globalVar) * 100

        # if int(ScreenLength_globalVar) < 20:
        #     screen_length = int(ScreenLength_globalVar) * 200
        # else:
        #     screen_length = int(ScreenLength_globalVar) * 100
        #
        # if int(ScreenWidth_globalVar) < 10:
        #     screen_width = int(ScreenWidth_globalVar) * 200
        # else:
        #     screen_width = int(ScreenWidth_globalVar) * 100

        # Perbarui tampilan posisi tag
        if float(gridValue) < 1:
            x1tag = abs((float(screen_length)/2) + (float(tag_x) * 1000))
            y1tag = abs((float(screen_width)/2) - (float(tag_y) * 1000))
        else:
            x1tag = abs((float(screen_length)/2) + (float(tag_x) * 100))
            y1tag = abs((float(screen_width)/2) - (float(tag_y) * 100))

        tag_x_formatted = "{:.2f}".format(tag_x)
        tag_y_formatted = "{:.2f}".format(tag_y)

        self.addTag(float(x1tag), float(y1tag), f"1st Tag ({tag_x_formatted},{tag_y_formatted})")

    def trilateration(self, anchor1, anchor2, anchor3, distance1, distance2, distance3):
        x1, y1 = anchor1
        x2, y2 = anchor2
        x3, y3 = anchor3

        A = 2 * (x2 - x1)
        B = 2 * (y2 - y1)
        C = distance1 ** 2 - distance2 ** 2 - x1 ** 2 + x2 ** 2 - y1 ** 2 + y2 ** 2

        D = 2 * (x3 - x2)
        E = 2 * (y3 - y2)
        F = distance2 ** 2 - distance3 ** 2 - x2 ** 2 + x3 ** 2 - y2 ** 2 + y3 ** 2

        x = (C * E - F * B) / (E * A - B * D)
        y = (C * D - A * F) / (B * D - A * E)

        return x, y

# # # # # # # #  # # # #  END HERE # # # # # # # # #  # # # #

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = IndoorPositioningApp()
    window.show()
    sys.exit(app.exec())


