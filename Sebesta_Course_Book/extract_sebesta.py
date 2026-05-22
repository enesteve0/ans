import sys
import subprocess
import os

def install_and_import(package):
    try:
        __import__(package)
        return True
    except ImportError:
        print(f"Installing {package}...")
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", package, "--break-system-packages"])
            return True
        except Exception as e:
            print(f"Failed to install {package}: {e}")
            return False

def main():
    pdf_path = "Concepts of Programming Languages - Sebesta.pdf"
    if not os.path.exists(pdf_path):
        print(f"Error: {pdf_path} not found.")
        return

    # Install pypdf if needed
    if not install_and_import("pypdf"):
        print("Could not import or install pypdf.")
        return

    import pypdf
    print(f"Loading {pdf_path}...")
    reader = pypdf.PdfReader(pdf_path)
    num_pages = len(reader.pages)
    print(f"Total pages: {num_pages}")

    # Define targets and search keywords
    targets = {
        "ch1_evaluation": ["Language Evaluation Criteria", "readability", "writability", "reliability"],
        "ch3_syntax_semantics": ["Describing Syntax and Semantics", "EBNF", "Operational Semantics", "Denotational Semantics"],
        "ch5_names_bindings": ["Names, Bindings, and Scopes", "static scoping", "dynamic scoping", "lifetime"],
        "ch6_data_types": ["Data Types", "Name Equivalence", "Structural Equivalence", "strong typing"],
        "ch7_expressions": ["Expressions and Assignment", "operator precedence", "coercion", "short-circuit evaluation"]
    }

    # Find pages for each target
    page_matches = {key: set() for key in targets}
    
    print("Scanning PDF pages for keywords...")
    for idx in range(num_pages):
        try:
            page = reader.pages[idx]
            text = page.extract_text()
            if not text:
                continue
            text_lower = text.lower()
            
            for key, keywords in targets.items():
                for kw in keywords:
                    if kw.lower() in text_lower:
                        page_matches[key].add(idx + 1)
        except Exception as e:
            # Skip corrupted pages
            pass

    # Dump the matched pages (with context window) to files
    for key, pages in page_matches.items():
        if not pages:
            print(f"No pages found for target {key}")
            continue
        
        # Sort and filter pages
        sorted_pages = sorted(list(pages))
        print(f"Target '{key}' matched on pages: {sorted_pages[:20]}... (total {len(sorted_pages)} pages)")
        
        out_path = f"sebesta_{key}.txt"
        print(f"Writing matched pages to {out_path}...")
        
        with open(out_path, "w", encoding="utf-8") as f:
            for p in sorted_pages:
                f.write(f"\n=========================================\n")
                f.write(f"--- SEBESTA PAGE {p} (Target: {key}) ---\n")
                f.write(f"=========================================\n\n")
                try:
                    f.write(reader.pages[p-1].extract_text() or "")
                except Exception as e:
                    f.write(f"[Error extracting page {p}: {e}]\n")
                    
        print(f"Finished writing {out_path}")

    print("Extraction completed successfully!")

if __name__ == "__main__":
    main()
