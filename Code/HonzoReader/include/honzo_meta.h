#pragma once

#include <cstddef>
#include <cstdint>

// Extract book title and author from a Honzo file.
// Uses HonzoFileReader internally (opens the file, reads META, closes).
// Returns true if metadata was found and at least one field populated.
// Extract title and author from an already-opened Honzo meta JSON string.
bool honzo_scan_meta_json(const char* json,
                          char* title, int title_max,
                          char* author, int author_max);

// Open a Honzo file and extract title/author from META.
bool honzo_extract_meta(const char* file_path,
                        char* title, int title_max,
                        char* author, int author_max);

// Extract the first heading from chapter text.
//   markup: 0 = Markdown, 1 = HTML
void honzo_extract_heading(const char* text, size_t len, uint8_t markup,
                           char* out, int max_len);