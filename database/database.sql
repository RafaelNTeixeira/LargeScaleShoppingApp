DROP TABLE IF EXISTS shopping_list;
DROP TABLE IF EXISTS shopping_list_items;

CREATE TABLE shopping_list (
    url TEXT NOT NULL PRIMARY KEY,       
    name TEXT, 
    owner TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    deleted BOOLEAN DEFAULT FALSE
);

CREATE TABLE shopping_list_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,  
    list_url TEXT NOT NULL,               
    item_name TEXT NOT NULL,       
    acquired_quantity INTEGER DEFAULT 0,  
    added_by TEXT,    
    total_added INTEGER DEFAULT 0,        
    total_deleted INTEGER DEFAULT 0,   
    FOREIGN KEY (list_url) REFERENCES shopping_list (url) ON DELETE CASCADE
);
