import sys, os, pypandoc, sqlite3
from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QComboBox, QMessageBox, QSpacerItem, QSizePolicy
from PyQt6.QtCore import QUrl
from PyQt6.QtWebEngineWidgets import QWebEngineView
from pylatex import Document, Section, Subsection, Tabularx, MultiColumn
from pylatex.utils import NoEscape


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
        geometry_options = {"margin": "2.54cm"}
        self.doc = Document(geometry_options=geometry_options)
        self.setMinimumSize(800,600)

        vlayout = QVBoxLayout()
        vlayout2 = QVBoxLayout()
        hlayout = QHBoxLayout()

        self.scenario_cb = QComboBox()
        self.add_data_to_combobox("database.db", self.scenario_cb, "`Scenario Name`", "Scenarios")
        self.contingency_cb = QComboBox()
        self.add_data_to_combobox("database.db", self.contingency_cb, "`Contingency Name`", "Contingency")

        report = QPushButton("Generate Report")
        report.clicked.connect(self.retrieve_data)
        
        scenario_label = QLabel('Scenario')
        scenario_label.setMaximumHeight(15)
        vlayout.addWidget(scenario_label)
        vlayout.addWidget(self.scenario_cb)
        contingency_label = QLabel('Contingency')
        contingency_label.setMaximumHeight(15)
        vlayout.addWidget(contingency_label)
        vlayout.addWidget(self.contingency_cb)
        vlayout.addWidget(report)
        verticalSpacer = QSpacerItem(20, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding)
        vlayout.addItem(verticalSpacer)
        self.webView = QWebEngineView()
        self.webView.settings().setAttribute(self.webView.settings().WebAttribute.PluginsEnabled, True)
        self.webView.settings().setAttribute(self.webView.settings().WebAttribute.PdfViewerEnabled, True)
        report_label = QLabel('Report')
        report_label.setMaximumHeight(15)

        vlayout2.addWidget(report_label)
        vlayout2.addWidget(self.webView)

        report_button = QPushButton("Save to Computer")
        report_button.clicked.connect(self.save_report)
        vlayout2.addWidget(report_button)
        hlayout.addLayout(vlayout, 1)
        hlayout.addLayout(vlayout2, 2)
        widget = QWidget()
        widget.setLayout(hlayout)
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

        self.doc.preamble.append(NoEscape(r'\title{Steady State Contingency Analysis Report\vspace{-3ex}}'))
        self.doc.preamble.append(NoEscape(r'\date{Report generated: \today\vspace{-2ex}}'))
        self.doc.append(NoEscape(r'\maketitle'))

        with self.doc.create(Section(f"Contingency: {self.contingency} (Islands created: ?)", False)):

            with self.doc.create(Subsection(f"Total number of monitored buses: {self.num_voltage}", False)):
                if (len(self.bus_data) != 0):
                    with self.doc.create(Tabularx('| p{2cm} | p{4 cm} | p{1.58cm} | p{2.1cm} | p{2.1cm} | p{2.1cm} |')) as bus_table:
                        
                    #with self.doc.create(Tabularx('|X|X|X|X|X|X|', width_argument=NoEscape(r'\columnwidth'))) as bus_table:
                        bus_table.add_hline()
                        bus_table.add_row([MultiColumn(6, align='|c|', data="Bus Voltage Violations")])
                        bus_table.add_hline()
                        bus_table.add_row(["Bus Number", "Bus Name", "Bus Base (kV)", "Low Voltage Criteria (pu)", "High Voltage Critera (pu)", "Bus Voltage (pu)"])
                    
                        for i in range(len(self.bus_data)):
                            bus_table.add_hline()
                            bus_table.add_row(self.bus_data[i][0], self.bus_data[i][2], self.bus_data[i][3], round(self.bus_data[i][4], 2), round(self.bus_data[i][5], 2), round(self.bus_data[i][1], 2))
                        bus_table.add_hline()
                else:
                    self.doc.append("No violations.")
               
            with self.doc.create(Subsection(f"Total number of monitored branches: {self.num_thermalbranch}", False)):
                if (len(self.branch_data) != 0):
                    total_width = NoEscape(r'\textwidth')
                    with self.doc.create(Tabularx('| p{2.51cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} |')) as branch_table:
                    #with self.doc.create(Tabularx("|c|c|c|c|c|c|c|c|", NoEscape(r'\textwidth'))) as branch_table:

                    
                        branch_table.add_hline()
                        branch_table.add_row([MultiColumn(8, align='|c|', data="Branch Thermal Violations")])
                        branch_table.add_hline()
                        branch_table.add_row(["Branch Name", "Metered End", "Other End", "Branch ID", "Voltage Class (kV)", "Rating (Amps)", "Metered End Loading (Amps)", "Other End Loading (Amps)"])
                    
                        for i in range(len(self.branch_data)):
                            branch_table.add_hline()
                            branch_table.add_row(self.branch_data[i][0], self.branch_data[i][3], self.branch_data[i][4], self.branch_data[i][5], self.branch_data[i][6], round(self.branch_data[i][8], 2) if self.season == "Winter" else round(self.branch_data[i][7], 2), round(self.branch_data[i][1], 2), round(self.branch_data[i][2], 2))
                        branch_table.add_hline()
                    
                else:
                    self.doc.append("No violations.")
    
    def display_report(self, tex_file):
        output = pypandoc.convert_file(tex_file, 'html', format='latex')
    # Write the output to the HTML file
        with open("report.html", 'w') as f:
            f.write(output)
        file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "report.html"))
        local_url = QUrl.fromLocalFile(file_path)
        self.webView.setUrl(local_url)

    def save_report(self):
        self.doc.generate_pdf('report', clean_tex=False)
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