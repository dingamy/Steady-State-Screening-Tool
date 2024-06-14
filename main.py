import sys
import sqlite3
from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow, QLabel, QLineEdit, QVBoxLayout, QMenu, QHBoxLayout, QGridLayout, QStackedLayout, QTableWidget, QTableWidgetItem
from PyQt6.QtCore import QSize, Qt
from PyQt6.QtGui import QAction, QPixmap, QPalette, QColor
import numpy as np
from matplotlib.figure import Figure
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
from matplotlib import pyplot as plt
from pylatex import Document, Section, Subsection, Command, Tabular, LongTable, PageStyle, Head, LargeText, MediumText, LineBreak, MiniPage, MultiColumn, Tabularx, HugeText
from pylatex.utils import italic, NoEscape, bold
import os
import io
import subprocess
import matplotlib.pyplot as plt
import io
from PIL import Image, ImageChops

from PyQt6.QtWidgets import QMainWindow, QApplication, QLabel, QCheckBox, QComboBox, QListWidget, QLineEdit, QLineEdit, QSpinBox, QDoubleSpinBox, QSlider, QMessageBox, QTextEdit

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
        self.branch_data = []
        self.doc = ""

        self.layout = QGridLayout()
        self.scenario_cb = QComboBox()
        self.add_data_to_combobox("database.db", self.scenario_cb, "`Scenario Name`", "Scenarios")
        self.contingency_cb = QComboBox()
        self.add_data_to_combobox("database.db", self.contingency_cb, "`Contingency Name`", "Contingency")
        report = QPushButton("Generate Report")
        report.clicked.connect(self.retrieve_data)
        self.layout.addWidget(QLabel('Scenario'), 0, 0)
        self.layout.addWidget(self.scenario_cb, 1, 0)
        self.layout.addWidget(QLabel('Contingency'), 2, 0)
        self.layout.addWidget(self.contingency_cb, 3, 0)
        self.layout.addWidget(report, 4, 0)
        self.text_edit = QLabel()
        self.layout.addWidget(QLabel("Report"), 0, 1)
        self.layout.addWidget(self.text_edit, 1, 1,)

        report_button = QPushButton("Save to Computer")
        report_button.clicked.connect(self.save_report)
        self.layout.addWidget(report_button, 4, 1)

        widget = QWidget()
        widget.setLayout(self.layout)
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

        self.display_report()
       
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

        

    def save_report(self):
        self.doc.generate_pdf('report', clean_tex=False)
        self.done_alert()

    def display_report(self):
        self.generate_report()
        self.doc.generate_tex('tex')
        #f = open("tex.tex", "r")
        #latex_text = f.read()
        #print(latex_text)

                
      
        with open("tex.tex", 'r', encoding='utf-8') as file:
            lines = file.readlines()
            latex_content = ""
            #for line in lines:
                
               # line = line.replace("%", "")
                #latex_content += line
                ##print(line)
            #print(latex_content)
            #subprocess.run(['pdflatex', 'tex.tex'], check=True)
            #latex_content = file.read()
            #print("haiii")
            #image = self.pdf_to_image('tex.pdf')
            #self.text_edit.setPixmap(image)
            
        image = self.latex_to_img(r'\frac{x}{y^2}')
            #self.text_edit.setText(latex_content)
        #rendered_content = self.render_latex(tex_content)
        #self.text_edit.setHtml(rendered_content)
        # Convert LaTeX to image
        #image = self.latex_to_image(latex_text)
           # self.text_edit.setPixmap(image)

        #delete tex.tex here
    def latex_to_img(self, tex):
        buf = io.BytesIO()
        plt.rc('text', usetex=True)
        plt.rc('font', family='serif')
        plt.axis('off')
        plt.text(0.05, 0.5, f'${tex}$', size=40)
        plt.savefig(buf, format='png')
        plt.close()

        im = Image.open(buf)
        bg = Image.new(im.mode, im.size, white)
        diff = ImageChops.difference(im, bg)
        diff = ImageChops.add(diff, diff, 2.0, -100)
        bbox = diff.getbbox()
        return im.crop(bbox)

    def latex_to_image(self, latex_text):
        plt.clf()
        plt.rcParams['text.usetex'] = True
        plt.rcParams['text.latex.preamble'] = r'\usepackage{amsmath}'
        print("latex_text", latex_text)
        fig, ax = plt.subplots()
        
        ax.text(0.5, 0.5, f"{latex_text}", fontsize=12)
        ax.axis('off')
        #print(ax.text);
        
        buf = io.BytesIO()
        '''
        plt.savefig(buf, format="png")
        buf.seek(0)
        
        image = QPixmap()
        image.loadFromData(buf.getvalue(), "PNG")
        buf.close()
        return image
		'''

    def pdf_to_image(self, pdf_path):
    # Open the PDF file
       # doc = fitz.open(pdf_path)
    
        # Get the first page
        page = doc.load_page(0)

        # Convert the first page to an image
        pix = page.get_pixmap()
    
        # Create a QPixmap from the image data
        image = QPixmap()
        image.loadFromData(pix.tobytes("png"), "PNG")
    
        return image
        

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