import sys
import sqlite3
from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow, QLabel, QLineEdit, QVBoxLayout, QMenu, QHBoxLayout, QGridLayout, QStackedLayout, QTableWidget, QTableWidgetItem
from PyQt6.QtCore import QSize, Qt
from PyQt6.QtGui import QAction, QPixmap, QPalette, QColor
import numpy as np

from pylatex import Document, Section, Subsection, Command, Tabular, LongTable, PageStyle, Head, LargeText, MediumText, LineBreak, MiniPage, MultiColumn, Tabularx, HugeText
from pylatex.utils import italic, NoEscape, bold
import os

from PyQt6.QtWidgets import (
    QMainWindow, QApplication,
    QLabel, QCheckBox, QComboBox, QListWidget, QLineEdit,
    QLineEdit, QSpinBox, QDoubleSpinBox, QSlider, QMessageBox
)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("<3 steady state screening tool ^w^")
        #self.width = 900
        #self.height = 800
        #self.setGeometry(0, 0, self.width, self.height)
        self.db = "database2.db"
        self.contingency = "";
        self.scenario = "";
        self.num_thermalbranch = 0
        self.num_voltage = 0
        self.bussim_result = []
        self.branch_table = []
        self.season = "";
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
        #self.contingency = "BRNDSHUNT";
        #self.scenario = "Base-sum2030-LGspcHH";
        conn = sqlite3.connect(self.db)
        cursor = conn.cursor()
        # GET THE SEASON
        query = f"SELECT `Season` FROM `Scenarios` WHERE `Scenario Name` = \"{self.scenario}\";"
        cursor.execute(query)
        self.season = cursor.fetchall()[0][0]

        # VOLTAGE TABLES
        query = f"SELECT `Bus Number`, bus_pu FROM `Bus Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\" and `Contingency Name` = \"{self.contingency}\" and violate = 1;"
        cursor.execute(query)
        self.bussim_result = cursor.fetchall()

        for i in range(len(self.bussim_result)):
            query = f"SELECT `Bus Name`, `Voltage Base`, `criteria_nlo`, `criteria_nhi` FROM BUS WHERE `Bus Number` = \"{self.bussim_result[i][0]}\";"
            cursor.execute(query)
            bus_result_part = cursor.fetchall()
            self.bussim_result[i] += bus_result_part[0]
        
        print(self.bussim_result)
        query = f"SELECT COUNT(`Bus Number`) FROM `BUS`;"
        cursor.execute(query)
        self.num_voltage = cursor.fetchall()[0][0]
        # BRANCH TABLES
        query = f"SELECT `Branch Name`, `amp_metered`, `amp_other` FROM `Branch Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\" and `Contingency Name` = \"{self.contingency}\" and violate = 1;"
        cursor.execute(query)
        self.branch_table = cursor.fetchall()

        for i in range(len(self.branch_table)):
            query = f"SELECT `Metered Bus Number`, `Other Bus Number`, `Branch ID`, `Voltage Base`, `RateA sum`, `RateA win` FROM `Branch` WHERE `Branch Name` = \"{self.branch_table[i][0]}\";"
            cursor.execute(query)
            branch_result_part = cursor.fetchall()
            self.branch_table[i] += branch_result_part[0]

        print(self.branch_table)
        query = f"SELECT COUNT(`Branch Name`) FROM `Branch`;"
        cursor.execute(query)
        self.num_thermalbranch = cursor.fetchall()[0][0]
       
    def generate_report(self):
        #geometry_options = {"tmargin" : "2.54cm", "margin": "2.54cm", "includeheadfoot": True, "papersize": "letterpaper"}
        geometry_options = {"margin": "2.54cm"}
        doc = Document(geometry_options=geometry_options)
       
        doc.preamble.append(NoEscape(r'\title{Steady State Contingency Analysis Report\vspace{-3ex}}'))
        doc.preamble.append(NoEscape(r'\date{Report generated: \today\vspace{-2ex}}'))
        doc.append(NoEscape(r'\maketitle'))
        #doc.append(NoEscape(r'\noindent'))
       # doc.append(LargeText(bold("Steady State Contingency Analysis Report")))
        #doc.append("\n")
       # doc.append(MediumText("Report Generated: "))
       # doc.append(MediumText(NoEscape(r'\today')))
        #with doc.create(MiniPage(align='c')):
        #doc.append('Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut ut euismod eros. Pellentesque porta velit non commodo molestie. Sed ipsum lacus, aliquet at venenatis quis, commodo vitae arcu. Fusce arcu lacus, ultrices sed dapibus a, auctor at metus. Cras nec finibus erat, at sagittis est. Vivamus sagittis elit nec lectus egestas molestie. Phasellus et venenatis turpis. Morbi hendrerit interdum urna, sed hendrerit libero ultrices at. Duis feugiat sodales eleifend. Aliquam sit amet congue quam. Nam et nulla id diam consequat scelerisque. Nullam pulvinar quam id nibh molestie, quis feugiat arcu tempor. Aliquam rhoncus suscipit ultricies.')
        
        with doc.create(Section(f"Contingency: {self.contingency} (Islands created: ?)", False)):

            with doc.create(Subsection(f"Total number of monitored buses: {self.num_voltage}", False)):
                if (len(self.bussim_result) != 0):
                    #with doc.create(Tabularx('| p{2cm} | p{3.68cm} | p{1.6cm} | p{2.30cm} | p{2.30cm} | p{2.30cm} |')) as bus_table:
                    with doc.create(Tabularx('|X|X|X|X|X|X|', width_argument=NoEscape(r'\columnwidth'))) as bus_table:
                        bus_table.add_hline()
                        bus_table.add_row(["Bus Number", "Bus Name", "Bus Base (kV)", "Low Voltage Criteria (pu)", "High Voltage Critera (pu)", "Bus Voltage (pu)"])
                        bus_table.add_hline()
                    
                        for i in range(len(self.bussim_result)):
                            bus_table.add_hline()
                            bus_table.add_row(self.bussim_result[i][0], self.bussim_result[i][2], self.bussim_result[i][3], self.bussim_result[i][4], self.bussim_result[i][5], self.bussim_result[i][1])
                        bus_table.add_hline()
                else:
                    doc.append("No violations.")
               
            with doc.create(Subsection(f"Total number of monitored branches: {self.num_thermalbranch}", False)):
                if (len(self.branch_table) != 0):
                    #with doc.create(Tabularx('| p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} |')) as branch_table_view:
                    with doc.create(Tabularx('|X|X|X|X|X|X|X|X|', width_argument=NoEscape(r'\columnwidth'))) as branch_table_view:
                        branch_table_view.add_hline()
                        branch_table_view.add_row(["Branch Name", "Metered End", "Other End", "Branch ID", "Voltage Class (kV)", "Rating (Amps)", "Metered End Loading (Amps)", "Other End Loading (Amps)"])
                        branch_table_view.add_hline()
                    
                        for i in range(len(self.branch_table)):
                            branch_table_view.add_hline()
                            branch_table_view.add_row(self.branch_table[i][0], self.branch_table[i][3], self.branch_table[i][4], self.branch_table[i][5], self.branch_table[i][6], self.branch_table[i][8] if self.season == "Winter" else self.branch_table[i][7], self.branch_table[i][1], self.branch_table[i][2])
                        branch_table_view.add_hline()
                else:
                    doc.append("No violations.")
            
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