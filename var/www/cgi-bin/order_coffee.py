#!/usr/bin/env python3

import cgi
import cgitb
import os
from urllib.parse import parse_qs

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
    print("Content-Type: text/html\n")

    # Get the query string
    query_string = os.environ.get("QUERY_STRING", "")
    params = parse_qs(query_string)

    # View mode (iframe to display all orders)
    if "view" in params:
        print("<pre>")
        for order in read_orders():
            print(order.strip())
        print("</pre>")
        return

    # Process form data
    form = cgi.FieldStorage()
    name = form.getvalue("name")
    coffee = form.getvalue("coffee")
    size = form.getvalue("size")

    if name and coffee and size:
        save_order(name, coffee, size)

    # Confirmation and link back
    print("<h1>Order received!</h1>")
    print(f"<p>Thank you, {name}. You ordered a {size} {coffee}.</p>")
    print('<p><a href="/order.html">Back to order form</a></p>')
    print('<p><a href="/index.html">Back to index page</a></p>')

if __name__ == "__main__":
    main()
