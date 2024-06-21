import sys, os, pypandoc, sqlite3
from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QComboBox, QMessageBox, QSpacerItem, QSizePolicy, QTreeWidget, QTreeWidgetItem
from PyQt6.QtCore import QUrl, Qt
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
        self.branch_data = []
        self.thermal_selected = []
        self.voltage_selected = []
        self.column_names = {
            "Bus Voltage": ["Bus Number", "Bus Name", "Bus Base (kV)", "Low Voltage Criteria (pu)", "High Voltage Critera (pu)", "Bus Voltage (pu)"],
            "Branch Thermal": ["Branch Name", "Metered End", "Other End", "Branch ID", "Voltage Class (kV)", "Rating (Amps)", "Metered End Loading (Amps)", "Other End Loading (Amps)"]
        }
        self.filtered_column_names = {
            "From Bus": "Metered End",
            "To Bus": "Other End",
            "Branch ID": "Branch ID",
            "Bus": "Bus Number"
        }
        self.bus_table_index = {
            "Bus Number": 0,
            "Bus Name": 2,
            "Bus Base (kV)": 3,
            "Low Voltage Criteria (pu)": 4,
            "High Voltage Critera (pu)": 5,
            "Bus Voltage (pu)": 1
        }
        self.branch_table_index = {
            "Branch Name": 0,
            "Metered End": 3,
            "Other End": 4,
            "Branch ID": 5,
            "Voltage Class (kV)": 6,
            "Rating (Amps)": 7,
            "Metered End Loading (Amps)": 1,
            "Other End Loading (Amps)": 2
        }

        geometry_options = {"margin": "2.54cm"}
        self.doc = Document(geometry_options=geometry_options)
        self.resize(800,600)

        vlayout = QVBoxLayout()
        vlayout2 = QVBoxLayout()
        hlayout = QHBoxLayout()
        
        scenario_label = QLabel('Scenario')
        scenario_label.setMaximumHeight(15)
        vlayout.addWidget(scenario_label)

        self.scenario_cb = QComboBox()
        self.add_data_to_combobox(self.scenario_cb, "`Scenario Name`", "Scenarios")
        vlayout.addWidget(self.scenario_cb)

        contingency_label = QLabel('Contingency')
        contingency_label.setMaximumHeight(15)
        vlayout.addWidget(contingency_label)

        self.contingency_cb = QComboBox()
        self.add_data_to_combobox(self.contingency_cb, "`Contingency Name`", "Contingency")
        vlayout.addWidget(self.contingency_cb)

        filter = QTreeWidget()
        filter.setHeaderLabel("Filter")

        self.thermal_filter = QTreeWidgetItem()
        filter.addTopLevelItem(self.thermal_filter)
        self.thermal_filter.setText(0, "Thermal")
        self.thermal_filter.setFlags(self.thermal_filter.flags() | Qt.ItemFlag.ItemIsAutoTristate | Qt.ItemFlag.ItemIsUserCheckable)
        from_bus_filter = QTreeWidgetItem()
        from_bus_filter.setFlags(from_bus_filter.flags() | Qt.ItemFlag.ItemIsUserCheckable)
        from_bus_filter.setText(0, "From Bus")
        from_bus_filter.setCheckState(0, Qt.CheckState.Checked)
        to_bus_filter = QTreeWidgetItem()
        to_bus_filter.setFlags(to_bus_filter.flags() | Qt.ItemFlag.ItemIsUserCheckable)
        to_bus_filter.setText(0, "To Bus")
        to_bus_filter.setCheckState(0, Qt.CheckState.Checked)
        branch_id_filter = QTreeWidgetItem()
        branch_id_filter.setFlags(branch_id_filter.flags() | Qt.ItemFlag.ItemIsUserCheckable)
        branch_id_filter.setText(0, "Branch ID")
        branch_id_filter.setCheckState(0, Qt.CheckState.Checked)
        self.thermal_filter.addChild(from_bus_filter)
        self.thermal_filter.addChild(to_bus_filter)
        self.thermal_filter.addChild(branch_id_filter)

        self.voltage_filter = QTreeWidgetItem()
        filter.addTopLevelItem(self.voltage_filter)
        self.voltage_filter.setText(0, "Voltage")
        self.voltage_filter.setFlags(self.voltage_filter.flags() | Qt.ItemFlag.ItemIsAutoTristate | Qt.ItemFlag.ItemIsUserCheckable)
        bus_filter = QTreeWidgetItem()
        bus_filter.setFlags(bus_filter.flags() | Qt.ItemFlag.ItemIsUserCheckable)
        bus_filter.setText(0, "Bus")
        bus_filter.setCheckState(0, Qt.CheckState.Checked)
        self.voltage_filter.addChild(bus_filter)
        vlayout.addWidget(filter)

        report = QPushButton("Generate Report")
        report.clicked.connect(self.retrieve_data)
        vlayout.addWidget(report)

        verticalSpacer = QSpacerItem(20, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding)
        vlayout.addItem(verticalSpacer)

        report_label = QLabel('Report')
        report_label.setMaximumHeight(15)
        vlayout2.addWidget(report_label)

        self.webView = QWebEngineView()
        self.webView.settings().setAttribute(self.webView.settings().WebAttribute.PluginsEnabled, True)
        self.webView.settings().setAttribute(self.webView.settings().WebAttribute.PdfViewerEnabled, True)
        vlayout2.addWidget(self.webView)

        report_button = QPushButton("Save to Computer")
        report_button.clicked.connect(self.save_report)
        vlayout2.addWidget(report_button)

        hlayout.addLayout(vlayout, 1)
        hlayout.addLayout(vlayout2, 2)
        widget = QWidget()
        widget.setLayout(hlayout)
        self.setCentralWidget(widget)
        
    def add_data_to_combobox(self, combobox, column, table):
        conn = sqlite3.connect(self.db)
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
        if self.season == "Winter":
            self.branch_table_index["Rating (Amps)" ] = 8
        else:
            self.branch_table_index["Rating (Amps)" ] = 7

        # VOLTAGE TABLES
        query = f"SELECT `Bus Number`, bus_pu FROM `Bus Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\" and `Contingency Name` = \"{self.contingency}\" and violate = 1 and exception = 0;"
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
        query = f"SELECT `Branch Name`, `amp_metered`, `amp_other` FROM `Branch Simulation Results` WHERE `Scenario Name` = \"{self.scenario}\" and `Contingency Name` = \"{self.contingency}\" and violate = 1 and exception = 0;"
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

        self.thermal_selected = []
        self.voltage_selected = []
        self.get_excluded_columns(self.thermal_filter, self.thermal_selected)
        self.get_excluded_columns(self.voltage_filter, self.voltage_selected)
        self.generate_report()
        self.doc.generate_tex("tex")
        self.display_report("tex.tex")
        conn.close()

    def get_excluded_columns(self, parent, checked_items):
        for i in range(0, parent.childCount()):
            if parent.child(i).checkState(0) == Qt.CheckState.Unchecked:
                checked_items.append(parent.child(i).text(0))

    def generate_report(self):
        geometry_options = {"margin": "2.54cm"}
        self.doc = Document(geometry_options=geometry_options)
        self.doc.preamble.append(NoEscape(r'\title{Steady State Contingency Analysis Report\vspace{-3ex}}'))
        self.doc.preamble.append(NoEscape(r'\date{Report generated: \today\vspace{-2ex}}'))
        self.doc.append(NoEscape(r'\maketitle'))

        with self.doc.create(Section(f"Contingency: {self.contingency} (Islands created: ?)", False)):
            with self.doc.create(Subsection(f"Total number of MVar margin criiteria screened: 0", False)):
                self.doc.append("No violations.")
            with self.doc.create(Subsection(f"Total number of monitored buses: {self.num_voltage}", False)):
                if (len(self.bus_data) != 0):
                    self.create_table("Bus Voltage", self.voltage_selected, self.bus_data, self.bus_table_index)
                else:
                    self.doc.append("No violations.")
            with self.doc.create(Subsection(f"Total number of monitored generators: 0", False)):
                self.doc.append("No violations.")
            with self.doc.create(Subsection(f"Total number of OOS margin criteria screened: 0", False)):
                self.doc.append("No violations.")
            with self.doc.create(Subsection(f"Total number of monitored branches: {self.num_thermalbranch}", False)):
                if (len(self.branch_data) != 0):
                    self.create_table("Branch Thermal", self.thermal_selected, self.branch_data, self.branch_table_index)
                else:
                    self.doc.append("No violations.")
            with self.doc.create(Subsection(f"Total number of monitored two winding transformers: 0", False)):
                self.doc.append("No violations.")
            with self.doc.create(Subsection(f"Total number of monitored three winding transformers: 0", False)):
                self.doc.append("No violations.")

    def create_table(self, table_name, filtered_columns, data, index):
        columns = self.column_names[table_name].copy()
        for i in range(len(filtered_columns)):
            columns.remove(self.filtered_column_names[filtered_columns[i]])
        table_str = '| '
        for i in range(len(columns)):
            if columns[i] == "Branch Name":
                table_str += 'p{3cm} |'
            elif columns[i] == "Bus Name":
                table_str += 'p{4cm} |'
            else:
                table_str += ' X |'
        with self.doc.create(Tabularx(table_str)) as table:
            table.add_hline()
            table.add_row([MultiColumn(len(columns), align='|c|', data=f"{table_name} Violations")])
            table.add_hline()
            table.add_row(columns)
            for i in range(len(data)):
                table.add_hline()
                data_row = []
                for j in range(len(columns)):
                    data_row.append(data[i][index[columns[j]]])
                table.add_row(data_row)
                table.add_hline()
            # | p{2cm} | p{4 cm} | p{1.58cm} | p{2.1cm} | p{2.1cm} | p{2.1cm} |' 
            # p{2.51cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} | p{1.5cm} |'
                    
    def display_report(self, tex_file):
        output = pypandoc.convert_file(tex_file, 'html', format='latex')
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
        #delete tex.tex file
        dialog.exec();


if __name__ == '__main__':
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    app.exec()