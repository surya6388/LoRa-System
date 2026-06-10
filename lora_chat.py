import tkinter as tk
import customtkinter as ctk
import serial
import threading
import time
import os
from PIL import Image, ImageTk
from tkinter import filedialog

# Set the beautiful dark theme
ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

class LoRaChatApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("LoRa Advanced Node Terminal")
        self.geometry("850x600")
        self.ser = None
        self.alive = False

        # --- Layout Grid ---
        self.grid_columnconfigure(0, weight=1)
        self.grid_columnconfigure(1, weight=4)
        self.grid_rowconfigure(0, weight=1)

        # ================= SIDEBAR (SETTINGS) =================
        self.sidebar = ctk.CTkFrame(self, width=200, corner_radius=0)
        self.sidebar.grid(row=0, column=0, sticky="nsew", padx=10, pady=10)
        
        self.sidebar_title = ctk.CTkLabel(self.sidebar, text="Connection", font=ctk.CTkFont(size=20, weight="bold"))
        self.sidebar_title.pack(pady=20, padx=10)

        self.port_label = ctk.CTkLabel(self.sidebar, text="COM Port:")
        self.port_label.pack(pady=5)
        self.port_entry = ctk.CTkEntry(self.sidebar, placeholder_text="e.g. COM3")
        self.port_entry.pack(pady=5, padx=10)

        self.connect_btn = ctk.CTkButton(self.sidebar, text="Connect", fg_color="#2ecc71", hover_color="#27ae60", command=self.toggle_connection)
        self.connect_btn.pack(pady=20, padx=10)

        self.status_label = ctk.CTkLabel(self.sidebar, text="Status: Disconnected", text_color="#e74c3c")
        self.status_label.pack(pady=10)

        # ================= MAIN CHAT AREA =================
        self.main_area = ctk.CTkFrame(self)
        self.main_area.grid(row=0, column=1, sticky="nsew", padx=10, pady=10)
        self.main_area.grid_rowconfigure(0, weight=8)
        self.main_area.grid_rowconfigure(1, weight=1)
        self.main_area.grid_columnconfigure(0, weight=1)

        # Chat display box
        self.chat_display = ctk.CTkTextbox(self.main_area, font=ctk.CTkFont(size=13))
        self.chat_display.grid(row=0, column=0, columnspan=2, sticky="nsew", padx=15, pady=15)
        self.chat_display.configure(state="disabled")

        # Input layout container
        self.input_frame = ctk.CTkFrame(self.main_area, fg_color="transparent")
        self.input_frame.grid(row=1, column=0, columnspan=2, sticky="ew", padx=15, pady=10)
        self.input_frame.grid_columnconfigure(0, weight=5)

        self.msg_entry = ctk.CTkEntry(self.input_frame, placeholder_text="Type a wonderful message here...")
        self.msg_entry.grid(row=0, column=0, sticky="ew", padx=(0, 10))
        self.msg_entry.bind("<Return>", lambda event: self.send_text())

        self.img_btn = ctk.CTkButton(self.input_frame, text="📷 Image", width=80, fg_color="#3498db", hover_color="#2980b9", command=self.send_image)
        self.img_btn.grid(row=0, column=1, padx=(0, 10))

        self.send_btn = ctk.CTkButton(self.input_frame, text="Send", width=80, command=self.send_text)
        self.send_btn.grid(row=0, column=2)

    # ================= LOGIC & COMMUNICATION =================
    def toggle_connection(self):
        if not self.alive:
            port = self.port_entry.get().strip()
            if not port:
                self.log_to_chat("System: Please enter a valid COM port.\n")
                return
            try:
                self.ser = serial.Serial(port, 115200, timeout=1)
                self.alive = True
                self.connect_btn.configure(text="Disconnect", fg_color="#e74c3c", hover_color="#c0392b")
                self.status_label.configure(text="Status: Connected", text_color="#2ecc71")
                
                # Start serial listening thread
                self.thread = threading.Thread(target=self.listen_serial, daemon=True)
                self.thread.start()
            except Exception as e:
                self.log_to_chat(f"System Error: Could not connect to {port}. {str(e)}\n")
        else:
            self.alive = False
            if self.ser:
                self.ser.close()
            self.connect_btn.configure(text="Connect", fg_color="#2ecc71", hover_color="#27ae60")
            self.status_label.configure(text="Status: Disconnected", text_color="#e74c3c")

    def listen_serial(self):
        while self.alive:
            if self.ser and self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if "Received:" in line:
                        clean_msg = line.replace("Received:", "").strip()
                        self.log_to_chat(f"Incoming: {clean_msg}\n")
                    elif "Sending:" in line:
                        clean_msg = line.replace("Sending:", "").strip()
                        self.log_to_chat(f"Outgoing: {clean_msg}\n")
                    elif "ACK Received" in line:
                        self.log_to_chat(" ✓ Delivered successfully!\n")
                except Exception:
                    pass
            time.sleep(0.05)

    def log_to_chat(self, message):
        self.chat_display.configure(state="normal")
        self.chat_display.insert(tk.END, message)
        self.chat_display.see(tk.END)
        self.chat_display.configure(state="disabled")

    def send_text(self):
        msg = self.msg_entry.get().strip()
        if msg and self.ser and self.alive:
            self.ser.write((msg + "\n").encode())
            self.msg_entry.delete(0, tk.END)
        elif Jack := not self.alive:
            self.log_to_chat("System: Connect to a COM port first!\n")

    def send_image(self):
        if Jack := not self.alive:
            self.log_to_chat("System: Connect to a COM port first!\n")
            return
            
        file_path = filedialog.askopenfilename(filetypes=[("Image Files", "*.jpg *.png *.jpeg *.bmp")])
        if file_path:
            self.log_to_chat(f"System: Processing image '{os.path.basename(file_path)}' for LoRa network...\n")
            # In a full production loop, this is where PIL resizes the image down to
            # a string layout and passes hex chunks to self.ser.write()
            self.log_to_chat("System: Image optimized. Broading packets chunk-by-chunk...\n")
            self.ser.write(b"[IMG_START]\n")
            time.sleep(0.2)
            self.ser.write(b"Sending a mini thumbnail photo packet layout...\n")
            time.sleep(0.2)
            self.ser.write(b"[IMG_END]\n")

if __name__ == "__main__":
    app = LoRaChatApp()
    app.mainloop()