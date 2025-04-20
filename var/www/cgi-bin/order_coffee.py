#!/usr/bin/env python3

import cgi
import cgitb
import os
import html
from urllib.parse import parse_qs
from datetime import datetime
import email.utils

cgitb.enable()

orders_file = "/tmp/coffee_orders.txt"

def save_order(name, coffee, size):
    with open(orders_file, "a") as f:
        f.write(f"{name} ordered a {size} {coffee}\n")

def read_orders():
    if not os.path.exists(orders_file):
        return []
    with open(orders_file, "r") as f:
        return f.readlines()

def main():
    # Get the query string
    query_string = os.environ.get("QUERY_STRING", "")
    params = parse_qs(query_string)
    
    # Process form data
    form = cgi.FieldStorage()
    name = html.escape(form.getvalue("name", ""))
    coffee = html.escape(form.getvalue("coffee", ""))
    size = html.escape(form.getvalue("size", ""))

    if name and coffee and size:
        save_order(name, coffee, size)

    # HTML response body
    html_body = f"""<!DOCTYPE html>
<html>
  <head>
    <title>Order Received</title>
    <link rel="icon" type="image/png" href="/favicon.png" />
    <link rel="stylesheet" type="text/css" href="/style/coffee.css" />
  </head>
  <body>
    <h1>Order received!</h1>
    <p>Thank you, {name}. You ordered a {size} {coffee}.</p>
    <p><a href="/index.html">Back to order form</a></p>
  </body>
</html>
"""

    # Generate headers
    date_str = email.utils.formatdate(usegmt=True)
    content_length = len(html_body.encode("utf-8"))

    print(f"HTTP/1.1 200 OK\r\n"
          f"Date: {date_str}\r\n"
          f"Content-Type: text/html\r\n"
          f"Content-Length: {content_length}\r\n"
          f"\r\n"
          f"{html_body}")

if __name__ == "__main__":
    main()
