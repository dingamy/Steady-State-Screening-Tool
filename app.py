from PyQt6.QtWidgets import QApplication, QLabel, QWidget, QGridLayout, QLineEdit, QPushButton
import sys
from datetime import datetime

class AgeCalculator(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Age Calculator")
        grid = QGridLayout()
        name_label = QLabel("Name:")
        self.name_line_edit = QLineEdit()

        date_label = QLabel("Date of Birth MM/DD/YYYY:")
        self.date_line_edit = QLineEdit()

        button = QPushButton("Calculate age")
        button.clicked.connect(self.calc_age)
        self.output = QLabel("")
        

        grid.addWidget(name_label, 0, 0)
        grid.addWidget(self.name_line_edit, 0, 1)
        grid.addWidget(date_label, 1, 0)
        grid.addWidget(self.date_line_edit, 1, 1)
        grid.addWidget(button, 2, 0, 1, 2)
        grid.addWidget(self.output, 3, 0, 1, 2)
        self.setLayout(grid)

    def calc_age(self):
        now = datetime.now().year
        bday = self.date_line_edit.text()
        yob = datetime.strptime(bday, "%m/%d/%Y").date().year
        age = now - yob
        self.output.setText(f"{self.name_line_edit.text()} is {age} years old")


app = QApplication(sys.argv)
age = AgeCalculator()
age.show()
sys.exit(app.exec())