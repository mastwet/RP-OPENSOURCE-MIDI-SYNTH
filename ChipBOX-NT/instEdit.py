import tkinter as tk
from tkinter import Menu, filedialog, messagebox, Scale, Label, Entry, Button

class InstrumentEditor(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Instrument Editor - 00. (2A03)")
        self.geometry("1074x768")
        self.create_menu()
        self.create_widgets()

    def create_menu(self):
        menu_bar = Menu(self)
        self.config(menu=menu_bar)

        file_menu = Menu(menu_bar, tearoff=0)
        file_menu.add_command(label="Open", command=self.open_file)
        file_menu.add_command(label="Save", command=self.save_file)
        menu_bar.add_cascade(label="File", menu=file_menu)

    def create_widgets(self):
        # Left panel for instrument settings
        left_panel = tk.Frame(self, bd=2, padx=5, pady=5, relief=tk.RIDGE)
        left_panel.pack(side=tk.LEFT, fill=tk.Y, expand=False)

        instrument_settings = tk.LabelFrame(left_panel, text="Instrument Settings", padx=5, pady=5)
        instrument_settings.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)

        # Sequence editor in the main area
        self.sequence_editor = tk.Canvas(self, width=600, height=300, bg='black')
        self.sequence_editor.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.sequence_editor.bind("<Button-1>", self.draw_point)
        self.sequence_editor.bind("<B1-Motion>", self.draw_point)

        # Control area below the canvas
        control_panel = tk.Frame(self, bd=2, padx=5, pady=5)
        control_panel.pack(side=tk.TOP, fill=tk.X, expand=False)

        self.length_label = Label(control_panel, text="Length:")
        self.length_label.pack(side=tk.LEFT, padx=5)

        self.length_entry = Entry(control_panel, width=5)
        self.length_entry.pack(side=tk.LEFT, padx=5)

        self.mode_label = Label(control_panel, text="Mode:")
        self.mode_label.pack(side=tk.LEFT, padx=5)

        self.mode_button = Button(control_panel, text="Toggle Mode", command=self.toggle_mode)
        self.mode_button.pack(side=tk.LEFT, padx=5)

    def draw_point(self, event):
        size = 6  # Size of the square
        self.sequence_editor.create_rectangle(event.x - size//2, event.y - size//2,
                                              event.x + size//2, event.y + size//2,
                                              fill='white', outline='')

    def toggle_mode(self):
        current_mode = self.mode_button.cget('text')
        if "Single" in current_mode:
            self.mode_button.config(text="Toggle Mode: Continuous")
        else:
            self.mode_button.config(text="Toggle Mode: Single")

    def open_file(self):
        filepath = filedialog.askopenfilename(filetypes=[("Binary files", "*.dat")])
        if filepath:
            messagebox.showinfo("Info", "File loaded successfully!")

    def save_file(self):
        filepath = filedialog.asksaveasfilename(defaultextension=".dat", filetypes=[("Binary files", "*.dat")])
        if filepath:
            messagebox.showinfo("Info", "File saved successfully!")

def main():
    app = InstrumentEditor()
    app.mainloop()

if __name__ == "__main__":
    main()
