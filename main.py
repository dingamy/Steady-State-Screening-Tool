import sys
import sqlite3
from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow, QLabel, QLineEdit, QVBoxLayout, QMenu, QHBoxLayout, QGridLayout, QStackedLayout, QTableWidget, QTableWidgetItem, QSizePolicy
from PyQt6.QtCore import QSize, Qt, QUrl
from PyQt6.QtGui import QAction, QPixmap, QPalette, QColor
from pylatex import Document, Section, Subsection, Command, Tabular, LongTable, PageStyle, Head, LargeText, MediumText, LineBreak, MiniPage, MultiColumn, Tabularx, HugeText
from pylatex.utils import italic, NoEscape, bold
import os
import io
import subprocess
import pypandoc

from PyQt6.QtWidgets import QMainWindow, QApplication, QLabel, QCheckBox, QComboBox, QListWidget, QLineEdit, QLineEdit, QSpinBox, QDoubleSpinBox, QSlider, QMessageBox, QTextEdit, QSpacerItem
from PyQt6.QtWebEngineWidgets import QWebEngineView

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("<3 steady state screening tool ^w^")
        self.db = "database2.db"
        self.contingency = "";
        self.scenario = "";
        self.season = "";
        self.num_thermalbranch = 0
        self.num_voltage = 0
        self.bus_data = []
        self.bus_data2 = []
        self.branch_data = []
        self.doc = ""
        self.vlayout = QVBoxLayout()
    
        self.vlayout2 = QVBoxLayout()
        self.hlayout = QHBoxLayout()
        self.scenario_cb = QComboBox()

        self.add_data_to_combobox("database.db", self.scenario_cb, "`Scenario Name`", "Scenarios")
        self.contingency_cb = QComboBox()
  
        self.add_data_to_combobox("database.db", self.contingency_cb, "`Contingency Name`", "Contingency")
        report = QPushButton("Generate and Save Report")
        report.clicked.connect(self.retrieve_data)
        
        scenario_label = QLabel('Scenario')
        scenario_label.setMaximumHeight(15)
        self.vlayout.addWidget(scenario_label)
        self.vlayout.addWidget(self.scenario_cb)
        contingency_label = QLabel('Contingency')
        contingency_label.setMaximumHeight(15)
        self.vlayout.addWidget(contingency_label)
        self.vlayout.addWidget(self.contingency_cb)
        self.vlayout.addWidget(report)
        verticalSpacer = QSpacerItem(20, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding)
        self.vlayout.addItem(verticalSpacer)
        self.webView = QWebEngineView()
        self.webView.settings().setAttribute(self.webView.settings().WebAttribute.PluginsEnabled, True)
        self.webView.settings().setAttribute(self.webView.settings().WebAttribute.PdfViewerEnabled, True)
        report_label = QLabel('Report')
        report_label.setMaximumHeight(15)

        self.vlayout2.addWidget(report_label)
        self.vlayout2.addWidget(self.webView)

        self.hlayout.addLayout(self.vlayout)
        self.hlayout.addLayout(self.vlayout2)
        widget = QWidget()
        widget.setLayout(self.hlayout)
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
        if (self.contingency == "None"):
            query = f"SELECT `Scenario Name`, `Contingency Name` FROM `Bus Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\";"
            cursor.execute(query)
            self.bus_data2 = cursor.fetchall()
            print(self.bus_data2)
        else:
            # GET THE SEASON
            query = f"SELECT `Season` FROM `Scenarios` WHERE `Scenario Name` = \"{self.scenario}\";"
            cursor.execute(query)
            self.season = cursor.fetchall()[0][0]

            # VOLTAGE TABLES
            query = f"SELECT `Bus Number`, bus_pu FROM `Bus Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\" and `Contingency Name` = \"{self.contingency}\" and violate = 1;"
            cursor.execute(query)
            self.bus_data = cursor.fetchall()

            for i in range(len(self.bus_data)):
                query = f"SELECT `Bus Name`, `Voltage Base`, `criteria_nlo`, `criteria_nhi` FROM BUS WHERE `Bus Number` = \"{self.bus_data[i][0]}\";"
                cursor.execute(query)
                bus_result_part = cursor.fetchall()
                self.bus_data[i] += bus_result_part[0]
        
            query = f"SELECT COUNT(`Bus Number`) FROM `BUS`;"
            cursor.execute(query)
            self.num_voltage = cursor.fetchall()[0][0]
            # BRANCH TABLES
            query = f"SELECT `Branch Name`, `amp_metered`, `amp_other` FROM `Branch Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\" and `Contingency Name` = \"{self.contingency}\" and violate = 1;"
            cursor.execute(query)
            self.branch_data = cursor.fetchall()

            for i in range(len(self.branch_data)):
                query = f"SELECT `Metered Bus Number`, `Other Bus Number`, `Branch ID`, `Voltage Base`, `RateA sum`, `RateA win` FROM `Branch` WHERE `Branch Name` = \"{self.branch_data[i][0]}\";"
                cursor.execute(query)
                branch_result_part = cursor.fetchall()
                self.branch_data[i] += branch_result_part[0]
            query = f"SELECT COUNT(`Branch Name`) FROM `Branch`;"
            cursor.execute(query)
            self.num_thermalbranch = cursor.fetchall()[0][0]
            self.generate_report()
            #self.display_report()
            self.doc.generate_tex("tex")
            self.display_report("tex.tex")
      
    def generate_report(self):
        geometry_options = {"margin": "2.54cm"}
        self.doc = Document(geometry_options=geometry_options)
       
        self.doc.preamble.append(NoEscape(r'\title{Steady State Contingency Analysis Report\vspace{-3ex}}'))
        self.doc.preamble.append(NoEscape(r'\date{Report generated: \today\vspace{-2ex}}'))
        self.doc.append(NoEscape(r'\maketitle'))

        with self.doc.create(Section(f"Contingency: {self.contingency} (Islands created: ?)", False)):

            with self.doc.create(Subsection(f"Total number of monitored buses: {self.num_voltage}", False)):
                if (len(self.bus_data) != 0):
                    #with doc.create(Tabularx('| p{2cm} | p{3.68cm} | p{1.6cm} | p{2.30cm} | p{2.30cm} | p{2.30cm} |')) as bus_table:
                    with self.doc.create(Tabularx('|X|X|X|X|X|X|', width_argument=NoEscape(r'\columnwidth'))) as bus_table:
                        bus_table.add_hline()
                        bus_table.add_row(["Bus Number", "Bus Name", "Bus Base (kV)", "Low Voltage Criteria (pu)", "High Voltage Critera (pu)", "Bus Voltage (pu)"])
                        bus_table.add_hline()
                    
                        for i in range(len(self.bus_data)):
                            bus_table.add_hline()
                            bus_table.add_row(self.bus_data[i][0], self.bus_data[i][2], self.bus_data[i][3], self.bus_data[i][4], self.bus_data[i][5], self.bus_data[i][1])
                        bus_table.add_hline()
                else:
                    self.doc.append("No violations.")
               
            with self.doc.create(Subsection(f"Total number of monitored branches: {self.num_thermalbranch}", False)):
                if (len(self.branch_data) != 0):
                    #with doc.create(Tabularx('| p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} | p{1.7725cm} |')) as branch_table:
                    with self.doc.create(Tabularx('|X|X|X|X|X|X|X|X|', width_argument=NoEscape(r'\columnwidth'))) as branch_table:
                        branch_table.add_hline()
                        branch_table.add_row(["Branch Name", "Metered End", "Other End", "Branch ID", "Voltage Class (kV)", "Rating (Amps)", "Metered End Loading (Amps)", "Other End Loading (Amps)"])
                        branch_table.add_hline()
                    
                        for i in range(len(self.branch_data)):
                            branch_table.add_hline()
                            branch_table.add_row(self.branch_data[i][0], self.branch_data[i][3], self.branch_data[i][4], self.branch_data[i][5], self.branch_data[i][6], self.branch_data[i][8] if self.season == "Winter" else self.branch_data[i][7], self.branch_data[i][1], self.branch_data[i][2])
                        branch_table.add_hline()
                else:
                    self.doc.append("No violations.")
        self.doc.generate_pdf('report', clean_tex=False)
        self.done_alert()

    def display_report(self, path):
        file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), path))
        local_url = QUrl.fromLocalFile(file_path)
        self.webView.setUrl(local_url)

       
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