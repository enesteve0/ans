// =====================================================
//   Bookstore Inventory Management System (C++17)
// =====================================================
// Demonstrates:
//   - Classes, inheritance, virtual functions
//   - enum class, struct, templates
//   - STL containers (vector, map, set)
//   - Smart pointers (unique_ptr, shared_ptr)
//   - Lambdas, <algorithm>, range-based for
//   - auto, structured bindings, std::optional
//   - Operator overloading
//   - Exception handling
//   - Namespaces

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <optional>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <functional>
#include <chrono>
#include <ctime>
#include <sstream>
#include <cmath>

namespace bookstore {

// ---------- Enum class ----------
enum class Genre {
    Fiction,
    NonFiction,
    Science,
    History,
    Biography
};

std::string genre_to_string(Genre g) {
    switch (g) {
        case Genre::Fiction:    return "Fiction";
        case Genre::NonFiction: return "Non-Fiction";
        case Genre::Science:    return "Science";
        case Genre::History:    return "History";
        case Genre::Biography:  return "Biography";
    }
    return "Unknown";
}

// ---------- Struct with constructor & operator overloading ----------
struct Book {
    std::string title;
    std::string author;
    Genre       genre;
    double      price;
    int         stock;
    std::optional<std::string> isbn;
    std::vector<std::string>   tags;

    Book(std::string t, std::string a, Genre g, double p,
         std::optional<std::string> i = std::nullopt,
         std::vector<std::string> tg = {})
        : title(std::move(t)), author(std::move(a)), genre(g),
          price(p), stock(0), isbn(std::move(i)), tags(std::move(tg)) {}

    bool is_in_stock() const { return stock > 0; }

    // Overload << so we can print a Book directly
    friend std::ostream& operator<<(std::ostream& os, const Book& b) {
        os << "'" << b.title << "' by " << b.author
           << " ($" << std::fixed << std::setprecision(2) << b.price << ")";
        return os;
    }
};

// ---------- Function template ----------
template <typename T>
void print_line(const std::string& label, const T& value) {
    std::cout << label << ": " << value << "\n";
}

// ---------- Logging helper (mimics a decorator) ----------
void log_action(const std::string& action) {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);
    std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] -> "
              << action << "()\n";
}

// ---------- Main class ----------
class Inventory {
public:
    explicit Inventory(std::string name) : store_name_(std::move(name)) {}

    void add_book(std::shared_ptr<Book> book, int quantity = 1) {
        log_action("add_book");
        auto it = books_.find(book->title);
        if (it != books_.end()) {
            it->second->stock += quantity;
        } else {
            book->stock = quantity;
            books_[book->title] = book;
        }
        std::cout << "   Added " << quantity << "x " << *book << "\n";
    }

    double sell_book(const std::string& title, int quantity = 1) {
        log_action("sell_book");
        auto it = books_.find(title);
        if (it == books_.end())
            throw std::runtime_error("Book '" + title + "' not found in inventory");

        auto& book = it->second;
        if (book->stock < quantity)
            throw std::runtime_error("Only " + std::to_string(book->stock) +
                                     " copies of '" + title + "' available");

        book->stock -= quantity;
        double total = book->price * quantity;

        sales_history_.push_back({title, quantity, total});
        std::cout << "   Sold " << quantity << "x " << *book
                  << " for $" << std::fixed << std::setprecision(2) << total << "\n";
        return total;
    }

    std::vector<std::shared_ptr<Book>> search_by_genre(Genre g) const {
        std::vector<std::shared_ptr<Book>> result;
        for (const auto& [title, book] : books_) {     // structured binding
            if (book->genre == g) result.push_back(book);
        }
        return result;
    }

    double get_total_value() const {
        // Lambda + STL accumulate
        return std::accumulate(books_.begin(), books_.end(), 0.0,
            [](double acc, const auto& pair) {
                return acc + pair.second->price * pair.second->stock;
            });
    }

    const auto& books() const { return books_; }

    struct Sale { std::string title; int quantity; double total; };
    const std::vector<Sale>& sales() const { return sales_history_; }

private:
    std::string store_name_;
    std::map<std::string, std::shared_ptr<Book>> books_;
    std::vector<Sale> sales_history_;
};

}  // namespace bookstore

// ---------- Generator-like helper using a vector ----------
std::vector<long long> fibonacci(int n) {
    std::vector<long long> result;
    long long a = 0, b = 1;
    for (int i = 0; i < n; ++i) {
        result.push_back(a);
        long long tmp = a + b;
        a = b;
        b = tmp;
    }
    return result;
}

// ---------- Lambda + transform ----------
std::vector<std::pair<std::string, double>>
apply_discount(const std::vector<std::shared_ptr<bookstore::Book>>& books, double discount) {
    std::vector<std::pair<std::string, double>> result;
    auto discount_fn = [discount](double price) { return price * (1.0 - discount); };
    std::transform(books.begin(), books.end(), std::back_inserter(result),
        [&](const auto& b) {
            return std::make_pair(b->title, std::round(discount_fn(b->price) * 100) / 100);
        });
    return result;
}

int main() {
    using namespace bookstore;

    std::cout << std::string(55, '=') << "\n"
              << "       BOOKSTORE INVENTORY DEMO\n"
              << std::string(55, '=') << "\n";

    Inventory store("The Reading Corner");

    // Vector of smart pointers
    std::vector<std::shared_ptr<Book>> sample_books = {
        std::make_shared<Book>("The Great Gatsby", "F. Scott Fitzgerald",
                               Genre::Fiction, 12.99, "978-0743273565"),
        std::make_shared<Book>("Sapiens", "Yuval Noah Harari",
                               Genre::History, 18.50, std::nullopt,
                               std::vector<std::string>{"bestseller", "anthropology"}),
        std::make_shared<Book>("A Brief History of Time", "Stephen Hawking",
                               Genre::Science, 15.00),
        std::make_shared<Book>("1984", "George Orwell",
                               Genre::Fiction, 10.99, std::nullopt,
                               std::vector<std::string>{"classic", "dystopia"}),
        std::make_shared<Book>("Steve Jobs", "Walter Isaacson",
                               Genre::Biography, 22.00),
    };

    std::cout << "\n--- STOCKING THE STORE ---\n";
    for (const auto& book : sample_books) {
        store.add_book(book, 5);
    }

    std::cout << "\nTotal inventory value: $"
              << std::fixed << std::setprecision(2)
              << store.get_total_value() << "\n";

    // Exception handling
    std::cout << "\n--- SALES ---\n";
    std::vector<std::pair<std::string, int>> orders = {
        {"1984", 2}, {"Sapiens", 3}, {"Unknown Book", 1}, {"1984", 99}
    };
    for (const auto& [title, qty] : orders) {
        try {
            store.sell_book(title, qty);
        } catch (const std::exception& e) {
            std::cout << "   ! Error: " << e.what() << "\n";
        }
    }

    // Range-based for + member access
    std::cout << "\n--- FICTION BOOKS ---\n";
    for (const auto& book : store.search_by_genre(Genre::Fiction)) {
        std::string status = book->is_in_stock() ? "in stock" : "out of stock";
        std::cout << "   " << *book << " - " << status
                  << " (" << book->stock << " left)\n";
    }

    // Set to collect unique authors
    std::set<std::string> authors;
    for (const auto& [title, book] : store.books()) authors.insert(book->author);
    std::cout << "\nUnique authors: " << authors.size() << "\n";

    // Lambda + transform via apply_discount
    std::cout << "\n--- 20% OFF FICTION ---\n";
    for (const auto& [title, new_price] :
         apply_discount(store.search_by_genre(Genre::Fiction), 0.20)) {
        std::cout << "   " << title << ": $"
                  << std::fixed << std::setprecision(2) << new_price << "\n";
    }

    // Generator-like vector
    std::cout << "\n--- FIBONACCI (first 10) ---\n   [";
    auto fibs = fibonacci(10);
    for (size_t i = 0; i < fibs.size(); ++i) {
        std::cout << fibs[i] << (i + 1 < fibs.size() ? ", " : "");
    }
    std::cout << "]\n";

    // Formatted output
    double pi = 3.14159265359;
    std::cout << "\nPi: " << std::fixed << std::setprecision(4) << pi
              << " | Percent: " << std::setprecision(2) << (0.875 * 100) << "%"
              << " | Hex of 255: 0x" << std::hex << 255 << std::dec << "\n";

    // Conditional + auto type deduction
    if (auto n = store.books().size(); n > 3) {
        std::cout << "\nLarge inventory detected: " << n << " unique titles\n";
    }

    // Manual JSON-like output of sales history
    std::cout << "\n--- SALES HISTORY ---\n[\n";
    const auto& sales = store.sales();
    for (size_t i = 0; i < sales.size(); ++i) {
        const auto& s = sales[i];
        std::cout << "  { title: \"" << s.title << "\""
                  << ", quantity: " << s.quantity
                  << ", total: " << std::fixed << std::setprecision(2) << s.total
                  << " }" << (i + 1 < sales.size() ? "," : "") << "\n";
    }
    std::cout << "]\n";

    // Template usage
    print_line<std::string>("\nStore", "The Reading Corner");
    print_line<int>("Total titles", store.books().size());

    return 0;
}
