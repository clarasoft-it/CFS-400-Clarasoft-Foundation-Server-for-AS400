using System.Net;
using System.Net.Sockets;
using System.Text;
using System.IO;



namespace cfs_client
{
    partial class cfs_client
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        private Stream channel;
        private ASCIIEncoding encoding = new ASCIIEncoding();
        private int connState;
        private TcpClient client;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.send = new System.Windows.Forms.Button();
            this.exit = new System.Windows.Forms.Button();
            this.server = new System.Windows.Forms.TextBox();
            this.port = new System.Windows.Forms.TextBox();
            this.connect = new System.Windows.Forms.Button();
            this.sendText = new System.Windows.Forms.TextBox();
            this.recvText = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // send
            // 
            this.send.Location = new System.Drawing.Point(286, 82);
            this.send.Name = "send";
            this.send.Size = new System.Drawing.Size(94, 30);
            this.send.TabIndex = 5;
            this.send.Text = "Send";
            this.send.UseVisualStyleBackColor = true;
            this.send.Click += new System.EventHandler(this.send_Click);
            // 
            // exit
            // 
            this.exit.Location = new System.Drawing.Point(286, 358);
            this.exit.Name = "exit";
            this.exit.Size = new System.Drawing.Size(94, 30);
            this.exit.TabIndex = 7;
            this.exit.Text = "Exit";
            this.exit.UseVisualStyleBackColor = true;
            this.exit.Click += new System.EventHandler(this.exit_Click);
            // 
            // server
            // 
            this.server.Location = new System.Drawing.Point(35, 36);
            this.server.Name = "server";
            this.server.Size = new System.Drawing.Size(172, 20);
            this.server.TabIndex = 1;
            // 
            // port
            // 
            this.port.Location = new System.Drawing.Point(214, 35);
            this.port.Name = "port";
            this.port.Size = new System.Drawing.Size(66, 20);
            this.port.TabIndex = 2;
            // 
            // connect
            // 
            this.connect.Location = new System.Drawing.Point(286, 25);
            this.connect.Name = "connect";
            this.connect.Size = new System.Drawing.Size(94, 30);
            this.connect.TabIndex = 3;
            this.connect.Text = "Connect";
            this.connect.UseVisualStyleBackColor = true;
            this.connect.Click += new System.EventHandler(this.connect_Click);
            // 
            // sendText
            // 
            this.sendText.Location = new System.Drawing.Point(34, 88);
            this.sendText.Name = "sendText";
            this.sendText.Size = new System.Drawing.Size(246, 20);
            this.sendText.TabIndex = 4;
            // 
            // recvText
            // 
            this.recvText.Location = new System.Drawing.Point(34, 126);
            this.recvText.Multiline = true;
            this.recvText.Name = "recvText";
            this.recvText.Size = new System.Drawing.Size(346, 207);
            this.recvText.TabIndex = 6;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(32, 14);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(38, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "Server";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(211, 14);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(26, 13);
            this.label2.TabIndex = 8;
            this.label2.Text = "Port";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(414, 400);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.server);
            this.Controls.Add(this.port);
            this.Controls.Add(this.connect);
            this.Controls.Add(this.sendText);
            this.Controls.Add(this.send);
            this.Controls.Add(this.recvText);
            this.Controls.Add(this.exit);
            this.Name = "Form1";
            this.Text = "CFS echo client";
            this.ResumeLayout(false);
            this.PerformLayout();


            this.send.Enabled = false;

        }

        #endregion

        private System.Windows.Forms.Button send;
        private System.Windows.Forms.Button exit;
        private System.Windows.Forms.TextBox server;
        private System.Windows.Forms.TextBox port;
        private System.Windows.Forms.Button connect;
        private System.Windows.Forms.TextBox sendText;
        private System.Windows.Forms.TextBox recvText;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;


    }
}

