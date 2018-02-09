using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.IO;

 
namespace cfs_client
{
    public partial class cfs_client : Form
    {
        public cfs_client()
        {
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.EndSession);
            InitializeComponent();
        }

        private void EndSession(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.client.Close();
            Environment.Exit(0);
        }

        private void connect_Click(object sender, EventArgs e)
        {
            if (this.connState == 0)
            {
                this.connState = 1;

                try
                {
                    if (server.Text.Length > 0 && port.Text.Length > 0)
                    {
                        this.client = new TcpClient();
                        this.client.Connect(server.Text, int.Parse(port.Text));

                        this.channel = client.GetStream();

                        this.server.Enabled = false;
                        this.port.Enabled = false;
                        this.connect.Text = "Disconnect";
                        this.send.Enabled = true;
                        this.sendText.Text = String.Empty;
                        this.recvText.Text = String.Empty;
                        this.sendText.Focus();
                    }
                }
                catch (ArgumentNullException excp)
                {
                    MessageBox.Show("Failed to connect to server: invalid arguments",
                                    "CFS Client",
                                    MessageBoxButtons.OK);
                }
                catch (SocketException excp)
                {
                    MessageBox.Show("Failed to connect to server",
                                    "CFS Client",
                                    MessageBoxButtons.OK);
                }
            }
            else
            {
                this.client.Close();
                this.connState = 0;
                this.connect.Text = "Connect";
                this.send.Enabled = false;
                this.server.Enabled = true;
                this.port.Enabled = true;
            }

        }

        private void send_Click(object sender, EventArgs e)
        {
            Byte[] data = encoding.GetBytes(sendText.Text);

            if (data.Length > 0)
            {
                try
                {
                    this.channel.Write(data, 0, data.Length);
                    sendText.Text = String.Empty;

                    data = new Byte[256];
                    String response = String.Empty;

                    Int32 bytes = channel.Read(data, 0, data.Length);
                    response = encoding.GetString(data, 0, bytes);

                    this.recvText.Text += (response + Environment.NewLine);
                }
                catch (IOException exc)
                {
                    MessageBox.Show("Connection dropped",
                                    "CFS echo client",
                                    MessageBoxButtons.OK);
                }
            }

            this.sendText.Focus();
        }

        private void exit_Click(object sender, EventArgs e)
        {
            this.client.Close();
            this.Close();
        }
    }
}
