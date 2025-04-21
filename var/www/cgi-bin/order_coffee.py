#!/usr/bin/env python3

import cgi
import cgitb
import os
import html
from urllib.parse import parse_qs, unquote
import email.utils
import sys

cgitb.enable()

orders_file = "var/temp/coffee_orders.txt"

def save_order(name, coffee, size):
    if not os.path.exists("var/temp/"):
        os.makedirs("var/temp/")
    with open(orders_file, "a") as f:
        f.write(f"{name} is waiting for a {size} {coffee}\n")

def read_orders():
    if not os.path.exists(orders_file):
        return []
    with open(orders_file, "r") as f:
        return f.readlines()

def main():

    # Print environment variables
    # print("Environment Variables:")
    # for key in sorted(os.environ):
    #     print(f"{key}: {os.environ[key]}")

    # Read POST data from stdin
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    raw_data = sys.stdin.read(content_length)

    # Print the raw POST data (for debugging)
    # print("\nRaw POST data:")
    # print(raw_data)

    # URL-decode the raw POST data before parsing it
    decoded_data = unquote(raw_data)
    
    # Parse the form data
    post_data = parse_qs(decoded_data)

    # Extract individual fields (list format, use the first element if available)
    name = post_data.get('name', [''])[0]
    coffee = post_data.get('coffee', [''])[0]
    size = post_data.get('size', [''])[0]

    # print(f"\nParsed Data:")
    # print(f"Name: {name}")
    # print(f"Coffee: {coffee}")
    # print(f"Size: {size}")

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
        <p><a href="/order.html">Back to order form</a></p>
    </body>
</html>
"""

    # Generate headers
    date_str = email.utils.formatdate(usegmt=True)
    content_length = len(html_body.encode("utf-8"))

    # Send the response headers and body
    print(f"HTTP/1.1 200 OK\r\n"
          f"Date: {date_str}\r\n"
          f"Content-Type: text/html\r\n"
          f"Content-Length: {content_length}\r\n"
          f"\r\n"
          f"{html_body}")

if __name__ == "__main__":
    main()
