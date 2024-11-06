DROP TABLE IF EXISTS shopping_list;
DROP TABLE IF EXISTS shopping_list_items;

CREATE TABLE shopping_list (
    url TEXT NOT NULL PRIMARY KEY,       
    name TEXT, 
    owner TEXT,
    deleted BOOLEAN DEFAULT FALSE
);

CREATE TABLE shopping_list_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,  
    list_url TEXT NOT NULL,               
    item_name TEXT NOT NULL,                 
    target_quantity INTEGER DEFAULT 1,   
    acquired_quantity INTEGER DEFAULT 0, 
    acquired BOOLEAN DEFAULT FALSE,
    deleted BOOLEAN DEFAULT FALSE,     
    added_by TEXT,    
    FOREIGN KEY (list_url) REFERENCES shopping_list (url) ON DELETE CASCADE
);
