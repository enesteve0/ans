"""
Bookstore Inventory Management System
=====================================
A demonstration of various Python syntax features:
- Type hints, dataclasses, enums
- Classes, inheritance, properties
- Decorators, generators, lambdas
- Comprehensions (list, dict, set, generator)
- Exception handling
- f-strings, walrus operator, tuple unpacking
"""

import json
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from typing import Optional


# ---------- Enum ----------
class Genre(Enum):
    FICTION = "Fiction"
    NON_FICTION = "Non-Fiction"
    SCIENCE = "Science"
    HISTORY = "History"
    BIOGRAPHY = "Biography"


# ---------- Dataclass with type hints ----------
@dataclass
class Book:
    title: str
    author: str
    genre: Genre
    price: float
    stock: int = 0
    isbn: Optional[str] = None
    tags: list[str] = field(default_factory=list)

    def __str__(self) -> str:
        return f"'{self.title}' by {self.author} (${self.price:.2f})"

    @property
    def is_in_stock(self) -> bool:
        return self.stock > 0


# ---------- Decorator ----------
def log_action(func):
    """A decorator that logs function calls with timestamps."""

    def wrapper(*args, **kwargs):
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{timestamp}] -> {func.__name__}()")
        return func(*args, **kwargs)

    return wrapper


# ---------- Main class ----------
class Inventory:
    def __init__(self, store_name: str):
        self.store_name = store_name
        self.books: dict[str, Book] = {}
        self.sales_history: list[dict] = []

    @log_action
    def add_book(self, book: Book, quantity: int = 1) -> None:
        if book.title in self.books:
            self.books[book.title].stock += quantity
        else:
            book.stock = quantity
            self.books[book.title] = book
        print(f"   Added {quantity}x {book}")

    @log_action
    def sell_book(self, title: str, quantity: int = 1) -> float:
        if title not in self.books:
            raise ValueError(f"Book '{title}' not found in inventory")

        book = self.books[title]
        if book.stock < quantity:
            raise ValueError(f"Only {book.stock} copies of '{title}' available")

        book.stock -= quantity
        total = book.price * quantity

        self.sales_history.append(
            {
                "title": title,
                "quantity": quantity,
                "total": round(total, 2),
                "timestamp": datetime.now().isoformat(timespec="seconds"),
            }
        )

        print(f"   Sold {quantity}x {book} for ${total:.2f}")
        return total

    def search_by_genre(self, genre: Genre) -> list[Book]:
        # List comprehension with filter
        return [book for book in self.books.values() if book.genre == genre]

    def get_total_value(self) -> float:
        # Generator expression passed to sum()
        return sum(book.price * book.stock for book in self.books.values())

    def get_low_stock(self, threshold: int = 3) -> list[Book]:
        return [b for b in self.books.values() if 0 < b.stock <= threshold]


# ---------- Generator function ----------
def fibonacci(n: int):
    """Yield the first n Fibonacci numbers."""
    a, b = 0, 1
    for _ in range(n):
        yield a
        a, b = b, a + b


# ---------- Lambda + higher-order function ----------
def apply_discount(books: list[Book], discount: float) -> list[tuple[str, float]]:
    discount_fn = lambda price: price * (1 - discount)
    return [(book.title, round(discount_fn(book.price), 2)) for book in books]


def main():
    print("=" * 55)
    print("       BOOKSTORE INVENTORY DEMO")
    print("=" * 55)

    store = Inventory("The Reading Corner")

    # Sample data
    sample_books = [
        Book(
            "The Great Gatsby",
            "F. Scott Fitzgerald",
            Genre.FICTION,
            12.99,
            isbn="978-0743273565",
        ),
        Book(
            "Sapiens",
            "Yuval Noah Harari",
            Genre.HISTORY,
            18.50,
            tags=["bestseller", "anthropology"],
        ),
        Book("A Brief History of Time", "Stephen Hawking", Genre.SCIENCE, 15.00),
        Book(
            "1984", "George Orwell", Genre.FICTION, 10.99, tags=["classic", "dystopia"]
        ),
        Book("Steve Jobs", "Walter Isaacson", Genre.BIOGRAPHY, 22.00),
    ]

    # Loop through and add stock
    print("\n--- STOCKING THE STORE ---")
    for book in sample_books:
        store.add_book(book, quantity=5)

    print(f"\nTotal inventory value: ${store.get_total_value():.2f}")

    # Exception handling
    print("\n--- SALES ---")
    for title, qty in [("1984", 2), ("Sapiens", 3), ("Unknown Book", 1), ("1984", 99)]:
        try:
            store.sell_book(title, qty)
        except ValueError as e:
            print(f"   ! Error: {e}")

    # Search and conditional expression
    print("\n--- FICTION BOOKS ---")
    for book in store.search_by_genre(Genre.FICTION):
        status = "in stock" if book.is_in_stock else "out of stock"
        print(f"   {book} - {status} ({book.stock} left)")

    # Set comprehension
    all_authors = {book.author for book in store.books.values()}
    print(f"\nUnique authors: {len(all_authors)}")

    # Tuple unpacking with star expression
    first, *middle, last = "Yuval Noah Harari".split()
    print(f"First: {first}, Middle: {middle}, Last: {last}")

    # Lambda usage
    print("\n--- 20% OFF FICTION ---")
    for title, new_price in apply_discount(store.search_by_genre(Genre.FICTION), 0.20):
        print(f"   {title}: ${new_price}")

    # Generator
    print("\n--- FIBONACCI (first 10) ---")
    print(f"   {list(fibonacci(10))}")

    # f-string formatting tricks
    pi = 3.14159265359
    print(f"\nPi: {pi:.4f} | Percent: {0.875:.2%} | Hex of 255: {255:#x}")

    # Walrus operator (Python 3.8+)
    if (n := len(store.books)) > 3:
        print(f"\nLarge inventory detected: {n} unique titles")

    # Dict comprehension
    stock_map = {book.title: book.stock for book in store.books.values()}
    print(f"\nStock map: {stock_map}")

    # JSON serialization
    print("\n--- SALES HISTORY (JSON) ---")
    print(json.dumps(store.sales_history, indent=2))


if __name__ == "__main__":
    main()
