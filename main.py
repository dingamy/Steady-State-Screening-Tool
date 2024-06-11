import sys
import sqlite3
from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow, QLabel, QLineEdit, QVBoxLayout, QMenu, QHBoxLayout, QGridLayout, QStackedLayout, QTableWidget, QTableWidgetItem
from PyQt6.QtCore import QSize, Qt
from PyQt6.QtGui import QAction, QPixmap, QPalette, QColor
import numpy as np

from pylatex import Document, Section, Subsection, Command, Tabular
from pylatex.utils import italic, NoEscape
import os

from PyQt6.QtWidgets import (
    QMainWindow, QApplication,
    QLabel, QCheckBox, QComboBox, QListWidget, QLineEdit,
    QLineEdit, QSpinBox, QDoubleSpinBox, QSlider, QMessageBox
)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("<3 steady state screening tool ^w^ ")
        #self.width = 900
        #self.height = 800
        #self.setGeometry(0, 0, self.width, self.height)
        self.db = "database2.db"
        self.contingency = "";
        self.scenario = "";
        self.num_thermalbranch = 0
        self.num_voltage = 0
        self.bussim_result = []

        layout = QGridLayout()

        self.scenario_cb = QComboBox()
        self.add_data_to_combobox("database.db", self.scenario_cb, "`Scenario Name`", "Scenarios")
        self.contingency_cb = QComboBox()
        self.add_data_to_combobox("database.db", self.contingency_cb, "`Contingency Name`", "Contingency")
        report = QPushButton("Generate Report")
        report.clicked.connect(self.retrieve_data)
        layout.addWidget(QLabel('Scenario'), 0, 0)
        layout.addWidget(self.scenario_cb, 1, 0)
        layout.addWidget(QLabel('Contingency'), 2, 0)
        layout.addWidget(self.contingency_cb, 3, 0)
        layout.addWidget(report, 4, 0)
        self.table = QTableWidget()
        layout.addWidget(QLabel("Report"), 0, 1)
        layout.addWidget(self.table, 1, 1,)

        report_button = QPushButton("Save to Computer")
        report_button.clicked.connect(self.generate_report)
        layout.addWidget(report_button, 4, 1)

        widget = QWidget()
        widget.setLayout(layout)
        self.setCentralWidget(widget)
        
        
    def add_data_to_combobox(self, db, combobox, column, table):
        conn = sqlite3.connect(db)
        cursor = conn.cursor()
        query = f"SELECT {column} FROM {table}"
        cursor.execute(query)
        result = cursor.fetchall()
        if table == "Contingency":
            combobox.addItem("None")
        for row in result:
            combobox.addItem(row[0])
        conn.commit()
        conn.close()

    def retrieve_data(self):
        self.contingency = self.contingency_cb.currentText();
        self.scenario = self.scenario_cb.currentText();
        conn = sqlite3.connect(self.db)
        cursor = conn.cursor()
        query = f"SELECT `Bus Number`, bus_pu FROM `Bus Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\" and `Contingency Name` = \"{self.contingency}\" and violate = 1";
        cursor.execute(query)
        self.bussim_result = cursor.fetchall()
        print(self.bussim_result)

        for row in self.bussim_result:
            query = f"SELECT `Bus Number`, `Voltage Base`, criteria_nlo, criteria_nhi FROM BUS WHERE `Bus Number` = \"{row[0]}\";"
            cursor.execute(query)
            bus_result = cursor.fetchall()
            print(bus_result)

        query = f"SELECT COUNT(`Bus Number`) FROM `BUS`;"
        cursor.execute(query)
        self.num_voltage = cursor.fetchall()[0][0]

        query = f"SELECT COUNT(`Branch Name`) FROM `Branch`;"
        cursor.execute(query)
        self.num_thermalbranch = cursor.fetchall()[0][0]

        print(self.contingency)
        print(self.scenario)
        print(self.num_thermalbranch)
        print(self.num_voltage)
       
    def generate_report(self):
        doc = Document()
        doc.append(NoEscape(r'\title{Steady State Contingency Analysis Report}'))
        doc.append(NoEscape(r'\maketitle'))
        with doc.create(Section(f"Contingency: {self.contingency} (Islands created: ?)", False)):
            doc.append(f"Total number of monitored buses: {self.num_voltage}")
            doc.append("\n")
            if (len(self.bussim_result) != 0):
                with doc.create(Tabular('| l | l | l | l | l | l |')) as bus_table:
                    bus_table.add_hline()
                    bus_table.add_row(["Bus Number", "Bus Name", "Bus Base (kV)", "Low Voltage Criteria (pu)", "High Voltage Critera (pu)", "Bus Voltage (pu)"])
                    for i in range(len(self.bussim_result)):
                        bus_table.add_hline()
                        bus_table.add_row("hi", "hi", "hi", "hi", "hi", "hi")
                    bus_table.add_hline()
            doc.append("\n")
            doc.append(f"Total number of monitored branches: {self.num_thermalbranch}")
        doc.generate_pdf('report', clean_tex=False)
        self.done_alert()

    def done_alert(self):
        dialog = QMessageBox(self)
        dialog.setWindowTitle("=^.^=")
        dialog.setText("Report has been successfully generated!")
        dialog.exec();

if __name__ == '__main__':
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    app.exec()