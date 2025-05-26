#include <raylib.h>
#include <stdio.h>    // For file I/O (fopen, fprintf, fscanf), snprintf
#include <stdlib.h>   // For strtof, atoi, exit
#include <string.h>   // For strcmp, strcpy, strlen, strncpy, strtok, strchr
#include "raygui.h"

// --- Constants ---
#define MAX_ITEMS 5         // Maximum number of auction items the app can hold
#define MAX_USERS 10        // Maximum number of registered users
#define MAX_NAME_LENGTH 32  // Max length for item names, usernames, bidder names
#define MAX_DESC_LENGTH 128 // Max length for item descriptions
#define MAX_BIDDER_LENGTH 32 // Max length for bidder names
#define MAX_INPUT_CHARS 20  // Max characters for input fields (bid amount, bidder name, username, password)
// Using a simple unsigned int for our basic hash, so MAX_PASSWORD_HASH_LENGTH is not directly relevant for string representation
// but rather for the hash value itself. We'll use 12 for general buffer safety if ever converting hash to string.
#define MAX_PASSWORD_HASH_LENGTH 12

#define USERS_FILE "users.txt" // File to store user credentials

// --- Custom Colors (using Raylib's CLITERAL for direct color definition) ---
#define LIGHTGRAY_CUSTOM CLITERAL(Color){ 200, 200, 200, 255 } // Lighter gray for UI elements
#define DARKGRAY_CUSTOM  CLITERAL(Color){ 50, 50, 50, 255 }     // Darker gray for text/borders
#define GREEN_ACCEPT     CLITERAL(Color){ 0, 150, 0, 255 }     // Green for "Place Bid" button, "Sign Up"
#define RED_DECLINE      CLITERAL(Color){ 150, 0, 0, 255 }      // Red for "Cancel" button, "Logout"
#define BLUE_HIGHLIGHT   CLITERAL(Color){ 0, 120, 200, 255 }  // Blue for active input boxes, "Sign In"
#define YELLOW_WARNING   CLITERAL(Color){ 255, 200, 0, 255 }  // Yellow for temporary UI messages

// --- Enums for Application Screens/States ---
// This helps manage what the user sees and interacts with at any given time.
typedef enum AppScreen {
    SCREEN_AUTH_MENU = 0, // New: User chooses between Sign In or Sign Up
    SCREEN_SIGN_IN,       // New: User enters credentials to log in
    SCREEN_SIGN_UP,       // New: User registers a new account
    SCREEN_ITEM_LIST,     // Displays a list of all auction items (after login)
    SCREEN_ITEM_DETAILS,  // Shows detailed information for a selected item
    SCREEN_PLACE_BID      // Screen for entering a new bid
} AppScreen;

// --- Structures ---

// AuctionItem: Represents a single item up for auction
typedef struct AuctionItem {
    char name[MAX_NAME_LENGTH];      // Name of the item
    char description[MAX_DESC_LENGTH]; // Detailed description
    float currentBid;                  // The highest bid currently placed
    char highestBidder[MAX_BIDDER_LENGTH]; // Name of the highest bidder
    bool auctionClosed;                // True if the auction for this item is over
} AuctionItem;

// InputBox: A helper structure to manage a single text input field
typedef struct InputBox {
    Rectangle rect;              // Position and size of the input box
    char text[MAX_INPUT_CHARS + 1]; // Buffer to store input text (+1 for null terminator)
    int letterCount;             // Current number of characters in the text buffer
    bool active;                 // True if this input box is currently focused for typing
    Color borderColor;           // Color of the box's border (changes when active)
    bool isPassword;             // New: True if this is a password field (for masking input)
} InputBox;

// User: Represents a registered user account
typedef struct User {
    char username[MAX_NAME_LENGTH];      // User's chosen username
    unsigned int hashedPassword;          // New: Stores the hashed version of the password
} User;

// --- Global Variables ---
AuctionItem items[MAX_ITEMS]; // Array to hold all auction items
int itemCount = 0;            // Current number of active auction items
int selectedItemIndex = -1;   // Index of the item currently selected/viewed (-1 if none)
AppScreen currentScreen = SCREEN_AUTH_MENU; // The application starts at the authentication menu

// User management variables
User users[MAX_USERS];        // Array to hold registered users
int userCount = 0;            // Current number of registered users
char loggedInUsername[MAX_NAME_LENGTH] = ""; // Stores username of the currently logged-in user

// Input boxes for various screens (declared globally for easy access and reset)
InputBox bidAmountInput;
InputBox bidderNameInput;
InputBox signInUsernameInput;
InputBox signInPasswordInput;
InputBox signUpUsernameInput;
InputBox signUpPasswordInput;
InputBox signUpConfirmPasswordInput;

// UI Message display variables
char uiMessage[100] = "";     // Buffer to store temporary messages for the user
float uiMessageTimer = 0.0f;  // Timer for how long the message should be displayed
const float UI_MESSAGE_DURATION = 3.0f; // Message display duration in seconds

// --- Function Prototypes ---
// Declaring functions before their implementation allows for better code organization.
void InitAuctionData(); // Initializes dummy data for auction items
void DrawItemListItem(int index, int x, int y, int width, int height); // Draws a single item in the list view
void DrawInputBox(InputBox* box, const char* label); // Draws an input box with a label
void UpdateInputBox(InputBox* box); // Handles keyboard input for an active input box
bool IsMouseOver(Rectangle rect); // Checks if the mouse cursor is over a given rectangle
void ResetInputBoxes(); // Clears and deactivates all input boxes for a clean state
void SetUIMessage(const char* message); // Displays a temporary message to the user

// User Management Functions (New)
unsigned int HashPassword(const char* password); // Simple non-cryptographic hash function
bool LoadUsers(); // Loads user data from USERS_FILE
bool SaveUsers(); // Saves current user data to USERS_FILE
bool RegisterUser(const char* username, const char* password); // Registers a new user
bool AuthenticateUser(const char* username, const char* password); // Authenticates a user
bool UsernameExists(const char* username); // Checks if a username is already taken


// --- Main Program Entry Point ---
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;  // Width of the application window
    const int screenHeight = 600; // Height of the application window

    // Initialize the Raylib window
    InitWindow(screenWidth, screenHeight, "Raylib Public Auction App");
    SetTargetFPS(60); // Set the application to run at 60 frames-per-second for smooth animation

    InitAuctionData(); // Populate the initial set of auction items
    LoadUsers();       // Load existing users from the file (if any)

    // Initialize the properties of all input boxes
    // Bid screen inputs
    bidAmountInput = (InputBox){(Rectangle){ screenWidth / 2 - 100, 300, 200, 40 }, "", 0, false, DARKGRAY_CUSTOM, false};
    bidderNameInput = (InputBox){(Rectangle){ screenWidth / 2 - 100, 380, 200, 40 }, "", 0, false, DARKGRAY_CUSTOM, false};

    // Sign In screen inputs
    signInUsernameInput = (InputBox){(Rectangle){ screenWidth / 2 - 120, 250, 240, 40 }, "", 0, false, DARKGRAY_CUSTOM, false};
    signInPasswordInput = (InputBox){(Rectangle){ screenWidth / 2 - 120, 320, 240, 40 }, "", 0, false, DARKGRAY_CUSTOM, true}; // isPassword = true for masking

    // Sign Up screen inputs
    signUpUsernameInput = (InputBox){(Rectangle){ screenWidth / 2 - 120, 200, 240, 40 }, "", 0, false, DARKGRAY_CUSTOM, false};
    signUpPasswordInput = (InputBox){(Rectangle){ screenWidth / 2 - 120, 270, 240, 40 }, "", 0, false, DARKGRAY_CUSTOM, true}; // isPassword = true
    signUpConfirmPasswordInput = (InputBox){(Rectangle){ screenWidth / 2 - 120, 340, 240, 40 }, "", 0, false, DARKGRAY_CUSTOM, true}; // isPassword = true
    //--------------------------------------------------------------------------------------

    // Main application loop
    // This loop continues as long as the window is not closed (e.g., by clicking the close button or pressing ESC)
    while (!WindowShouldClose()) {
        // Update Logic (handles user input and changes in application state)
        //----------------------------------------------------------------------------------
        float dt = GetFrameTime(); // Get time elapsed since last frame for timer updates
        Vector2 mousePoint = GetMousePosition(); // Get current mouse cursor position

        // Update the UI message timer
        if (uiMessageTimer > 0) {
            uiMessageTimer -= dt;
            if (uiMessageTimer <= 0) {
                strcpy(uiMessage, ""); // Clear message when timer expires
            }
        }

        // State machine: Logic changes based on the current screen
        switch (currentScreen) {
            case SCREEN_AUTH_MENU: {
                // Define buttons for Sign In and Sign Up
                Rectangle signInButton = { screenWidth / 2 - 100, screenHeight / 2 - 50, 200, 50 };
                Rectangle signUpButton = { screenWidth / 2 - 100, screenHeight / 2 + 20, 200, 50 };

                // Check for button clicks
                if (IsMouseOver(signInButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentScreen = SCREEN_SIGN_IN; // Transition to Sign In screen
                    ResetInputBoxes(); // Clear input fields
                    SetUIMessage("Enter your credentials."); // Provide user feedback
                }
                if (IsMouseOver(signUpButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentScreen = SCREEN_SIGN_UP; // Transition to Sign Up screen
                    ResetInputBoxes(); // Clear input fields
                    SetUIMessage("Choose a username and password."); // Provide user feedback
                }
            } break; // End of SCREEN_AUTH_MENU case

            case SCREEN_SIGN_IN: {
                // Update input boxes for username and password
                UpdateInputBox(&signInUsernameInput);
                UpdateInputBox(&signInPasswordInput);

                // Define Login and Back buttons
                Rectangle loginButton = { screenWidth / 2 - 80, 400, 160, 50 };
                Rectangle backButton = { screenWidth / 2 - 80, 470, 160, 50 };

                // Handle Login button click
                if (IsMouseOver(loginButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (AuthenticateUser(signInUsernameInput.text, signInPasswordInput.text)) {
                        strcpy(loggedInUsername, signInUsernameInput.text); // Set the logged-in user
                        currentScreen = SCREEN_ITEM_LIST; // Go to item list screen
                        SetUIMessage(TextFormat("Welcome, %s!", loggedInUsername)); // Welcome message
                        ResetInputBoxes(); // Clear login fields
                    } else {
                        SetUIMessage("Login failed. Check username/password."); // Error message
                    }
                }
                // Handle Back button click
                if (IsMouseOver(backButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentScreen = SCREEN_AUTH_MENU; // Go back to authentication menu
                    ResetInputBoxes(); // Clear input fields
                    SetUIMessage(""); // Clear message
                }
            } break; // End of SCREEN_SIGN_IN case

            case SCREEN_SIGN_UP: {
                // Update input boxes for username, password, and confirm password
                UpdateInputBox(&signUpUsernameInput);
                UpdateInputBox(&signUpPasswordInput);
                UpdateInputBox(&signUpConfirmPasswordInput);

                // Define Register and Back buttons
                Rectangle registerButton = { screenWidth / 2 - 80, 410, 160, 50 };
                Rectangle backButton = { screenWidth / 2 - 80, 480, 160, 50 };

                // Handle Register button click
                if (IsMouseOver(registerButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    // Basic validation for username and password length
                    if (strlen(signUpUsernameInput.text) < 3 || strlen(signUpPasswordInput.text) < 5) {
                        SetUIMessage("Username (min 3 chars) / Password (min 5 chars) too short.");
                    } else if (strcmp(signUpPasswordInput.text, signUpConfirmPasswordInput.text) != 0) {
                        SetUIMessage("Passwords do not match!"); // Check if passwords match
                    } else if (UsernameExists(signUpUsernameInput.text)) {
                        SetUIMessage("Username already taken."); // Check for duplicate username
                    } else {
                        // Attempt to register the user
                        if (RegisterUser(signUpUsernameInput.text, signUpPasswordInput.text)) {
                            SetUIMessage("Registration successful! Please sign in."); // Success message
                            currentScreen = SCREEN_SIGN_IN; // Go to Sign In screen
                            ResetInputBoxes(); // Clear sign-up fields
                        } else {
                            SetUIMessage("Registration failed. Try again."); // Generic failure (shouldn't happen if checks are good)
                        }
                    }
                }
                // Handle Back button click
                if (IsMouseOver(backButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentScreen = SCREEN_AUTH_MENU; // Go back to authentication menu
                    ResetInputBoxes(); // Clear input fields
                    SetUIMessage(""); // Clear message
                }
            } break; // End of SCREEN_SIGN_UP case

            case SCREEN_ITEM_LIST: {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    // Check if any auction item in the list was clicked
                    for (int i = 0; i < itemCount; i++) {
                        Rectangle itemRect = { 50, 100 + i * 60, screenWidth - 100, 50 };
                        if (CheckCollisionPointRec(mousePoint, itemRect)) {
                            selectedItemIndex = i;      // Set the clicked item as selected
                            currentScreen = SCREEN_ITEM_DETAILS; // Transition to the item details screen
                            SetUIMessage(""); // Clear any previous message
                            break; // Exit loop once an item is found
                        }
                    }
                }
                // Handle Logout button click
                Rectangle logoutButtonRect = { screenWidth - 150, 20, 120, 40 };
                if (IsMouseOver(logoutButtonRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    strcpy(loggedInUsername, ""); // Clear logged in user
                    currentScreen = SCREEN_AUTH_MENU; // Go back to authentication menu
                    SetUIMessage("Logged out successfully."); // Confirmation message
                }
            } break; // End of SCREEN_ITEM_LIST case

            case SCREEN_ITEM_DETAILS: {
                // Define the "Back" button area
                Rectangle backButtonRect = { 50, screenHeight - 60, 120, 40 };
                if (IsMouseOver(backButtonRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentScreen = SCREEN_ITEM_LIST; // Go back to the item list
                    selectedItemIndex = -1; // Deselect the item
                    SetUIMessage(""); // Clear message
                }

                // Define the "Place Bid" button area
                // Only show and enable this button if an item is selected and its auction is still open
                if (selectedItemIndex != -1 && !items[selectedItemIndex].auctionClosed) {
                    Rectangle placeBidButtonRect = { screenWidth - 170, screenHeight - 60, 120, 40 };
                    if (IsMouseOver(placeBidButtonRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        currentScreen = SCREEN_PLACE_BID; // Transition to the place bid screen
                        ResetInputBoxes(); // Clear any previous input from the bid fields
                        SetUIMessage("Enter your bid and name."); // Prompt user
                    }
                }
            } break; // End of SCREEN_ITEM_DETAILS case

            case SCREEN_PLACE_BID: {
                // Update logic for both input boxes (handling typing, focus)
                UpdateInputBox(&bidAmountInput);
                UpdateInputBox(&bidderNameInput);

                // Define the "BID!" and "Cancel" button areas
                Rectangle bidButtonRect = { screenWidth / 2 - 120, 480, 100, 40 };
                Rectangle cancelButtonRect = { screenWidth / 2 + 20, 480, 100, 40 };

                // Handle "BID!" button click
                if (IsMouseOver(bidButtonRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    float newBid = strtof(bidAmountInput.text, NULL); // Convert string to float

                    // Validate bidder name and bid amount
                    if (strlen(bidderNameInput.text) == 0) {
                        SetUIMessage("Please enter your name to bid."); // Prompt for name
                    } else if (newBid <= items[selectedItemIndex].currentBid) {
                        SetUIMessage(TextFormat("Bid failed: $%.2f is not higher than current bid $%.2f", newBid, items[selectedItemIndex].currentBid)); // Bid too low
                    } else if (newBid > 999999999.0f) { // Prevent excessively large bids
                        SetUIMessage("Bid amount too large!");
                    }
                    else {
                        items[selectedItemIndex].currentBid = newBid; // Update current bid
                        strcpy(items[selectedItemIndex].highestBidder, bidderNameInput.text); // Update highest bidder
                        TraceLog(LOG_INFO, "BID PLACED: %s for %.2f by %s", items[selectedItemIndex].name, newBid, bidderNameInput.text);
                        currentScreen = SCREEN_ITEM_DETAILS; // Go back to item details after successful bid
                        SetUIMessage(TextFormat("Bid of $%.2f placed successfully by %s!", newBid, bidderNameInput.text)); // Success message
                    }
                }
                // Handle "Cancel" button click
                if (IsMouseOver(cancelButtonRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentScreen = SCREEN_ITEM_DETAILS; // Go back to item details without placing a bid
                    SetUIMessage(""); // Clear message
                }
            } break; // End of SCREEN_PLACE_BID case
        }
        //----------------------------------------------------------------------------------

        // Drawing Logic (renders elements on the screen)
        //----------------------------------------------------------------------------------
        BeginDrawing(); // Start drawing operations

            ClearBackground(RAYWHITE); // Clear the background with a light white color

            // Drawing changes based on the current screen
            switch (currentScreen) {
                case SCREEN_AUTH_MENU: {
                    DrawText("Welcome to the Auction!", GetScreenWidth() / 2 - MeasureText("Welcome to the Auction!", 40) / 2, 100, 40, DARKGRAY_CUSTOM);

                    // Draw Sign In button
                    Rectangle signInButton = { GetScreenWidth() / 2 - 100, GetScreenHeight() / 2 - 50, 200, 50 };
                    DrawRectangleRec(signInButton, BLUE_HIGHLIGHT);
                    DrawRectangleLinesEx(signInButton, 2, DARKGRAY_CUSTOM);
                    DrawText("Sign In", signInButton.x + signInButton.width / 2 - MeasureText("Sign In", 30) / 2, signInButton.y + 10, 30, RAYWHITE);

                    // Draw Sign Up button
                    Rectangle signUpButton = { GetScreenWidth() / 2 - 100, GetScreenHeight() / 2 + 20, 200, 50 };
                    DrawRectangleRec(signUpButton, GREEN_ACCEPT);
                    DrawRectangleLinesEx(signUpButton, 2, DARKGRAY_CUSTOM);
                    DrawText("Sign Up", signUpButton.x + signUpButton.width / 2 - MeasureText("Sign Up", 30) / 2, signUpButton.y + 10, 30, RAYWHITE);
                } break; // End of SCREEN_AUTH_MENU drawing

                case SCREEN_SIGN_IN: {
                    DrawText("Sign In", GetScreenWidth() / 2 - MeasureText("Sign In", 40) / 2, 100, 40, DARKGRAY_CUSTOM);
                    DrawInputBox(&signInUsernameInput, "Username:");
                    DrawInputBox(&signInPasswordInput, "Password:");

                    // Draw Login button
                    Rectangle loginButton = { GetScreenWidth() / 2 - 80, 400, 160, 50 };
                    DrawRectangleRec(loginButton, GREEN_ACCEPT);
                    DrawRectangleLinesEx(loginButton, 2, DARKGRAY_CUSTOM);
                    DrawText("Login", loginButton.x + loginButton.width / 2 - MeasureText("Login", 25) / 2, loginButton.y + 13, 25, RAYWHITE);

                    // Draw Back button
                    Rectangle backButton = { GetScreenWidth() / 2 - 80, 470, 160, 50 };
                    DrawRectangleRec(backButton, LIGHTGRAY_CUSTOM);
                    DrawRectangleLinesEx(backButton, 2, DARKGRAY_CUSTOM);
                    DrawText("Back", backButton.x + backButton.width / 2 - MeasureText("Back", 25) / 2, backButton.y + 13, 25, DARKGRAY_CUSTOM);
                } break; // End of SCREEN_SIGN_IN drawing

                case SCREEN_SIGN_UP: {
                    DrawText("Sign Up", GetScreenWidth() / 2 - MeasureText("Sign Up", 40) / 2, 100, 40, DARKGRAY_CUSTOM);
                    DrawInputBox(&signUpUsernameInput, "Username:");
                    DrawInputBox(&signUpPasswordInput, "Password:");
                    DrawInputBox(&signUpConfirmPasswordInput, "Confirm Password:");

                    // Draw Register button
                    Rectangle registerButton = { GetScreenWidth() / 2 - 80, 410, 160, 50 };
                    DrawRectangleRec(registerButton, GREEN_ACCEPT);
                    DrawRectangleLinesEx(registerButton, 2, DARKGRAY_CUSTOM);
                    DrawText("Register", registerButton.x + registerButton.width / 2 - MeasureText("Register", 25) / 2, registerButton.y + 13, 25, RAYWHITE);

                    // Draw Back button
                    Rectangle backButton = { GetScreenWidth() / 2 - 80, 480, 160, 50 };
                    DrawRectangleRec(backButton, LIGHTGRAY_CUSTOM);
                    DrawRectangleLinesEx(backButton, 2, DARKGRAY_CUSTOM);
                    DrawText("Back", backButton.x + backButton.width / 2 - MeasureText("Back", 25) / 2, backButton.y + 13, 25, DARKGRAY_CUSTOM);
                } break; // End of SCREEN_SIGN_UP drawing

                case SCREEN_ITEM_LIST: {
                    DrawText("Auction Items", GetScreenWidth() / 2 - MeasureText("Auction Items", 40) / 2, 30, 40, DARKGRAY_CUSTOM);
                    // Display logged-in username
                    DrawText(TextFormat("Logged in as: %s", loggedInUsername), 20, 20, 20, DARKGRAY_CUSTOM);

                    // Draw each auction item in the list
                    for (int i = 0; i < itemCount; i++) {
                        DrawItemListItem(i, 50, 100 + i * 60, GetScreenWidth() - 100, 50);
                    }
                    // Draw Logout button
                    Rectangle logoutButtonRect = { GetScreenWidth() - 150, 20, 120, 40 };
                    DrawRectangleRec(logoutButtonRect, RED_DECLINE);
                    DrawRectangleLinesEx(logoutButtonRect, 2, DARKGRAY_CUSTOM);
                    DrawText("Logout", logoutButtonRect.x + logoutButtonRect.width / 2 - MeasureText("Logout", 20) / 2, logoutButtonRect.y + 10, 20, RAYWHITE);

                } break; // End of SCREEN_ITEM_LIST drawing

                case SCREEN_ITEM_DETAILS: {
                    if (selectedItemIndex != -1) {
                        AuctionItem currentItem = items[selectedItemIndex];

                        DrawText(currentItem.name, GetScreenWidth() / 2 - MeasureText(currentItem.name, 40) / 2, 30, 40, DARKBLUE);
                        DrawText(TextFormat("Description: %s", currentItem.description), 50, 100, 20, BLACK);
                        DrawText(TextFormat("Current Bid: $%.2f", currentItem.currentBid), 50, 140, 25, GREEN);
                        DrawText(TextFormat("Highest Bidder: %s", currentItem.highestBidder), 50, 170, 25, BLUE);
                        DrawText(TextFormat("Status: %s", currentItem.auctionClosed ? "CLOSED" : "OPEN"), 50, 210, 25, currentItem.auctionClosed ? RED : GREEN);

                        // Draw "Back" button
                        Rectangle backButtonRect = { 50, GetScreenHeight() - 60, 120, 40 };
                        DrawRectangleRec(backButtonRect, LIGHTGRAY_CUSTOM);
                        DrawRectangleLinesEx(backButtonRect, 2, DARKGRAY_CUSTOM);
                        DrawText("Back", backButtonRect.x + backButtonRect.width / 2 - MeasureText("Back", 20) / 2, backButtonRect.y + 10, 20, DARKGRAY_CUSTOM);

                        // Draw "Place Bid" button (only if auction is open)
                        if (!currentItem.auctionClosed) {
                            Rectangle placeBidButtonRect = { GetScreenWidth() - 170, GetScreenHeight() - 60, 120, 40 };
                            DrawRectangleRec(placeBidButtonRect, GREEN_ACCEPT);
                            DrawRectangleLinesEx(placeBidButtonRect, 2, DARKGRAY_CUSTOM);
                            DrawText("Place Bid", placeBidButtonRect.x + placeBidButtonRect.width / 2 - MeasureText("Place Bid", 20) / 2, placeBidButtonRect.y + 10, 20, RAYWHITE);
                        }
                    } else {
                        DrawText("No item selected. This shouldn't happen!", 50, 100, 20, RED);
                    }
                } break; // End of SCREEN_ITEM_DETAILS drawing

                case SCREEN_PLACE_BID: {
                    DrawText("Place Your Bid", GetScreenWidth() / 2 - MeasureText("Place Your Bid", 40) / 2, 30, 40, DARKGRAY_CUSTOM);
                    DrawText(TextFormat("Item: %s", items[selectedItemIndex].name), GetScreenWidth() / 2 - MeasureText(TextFormat("Item: %s", items[selectedItemIndex].name), 25) / 2, 100, 25, BLACK);
                    DrawText(TextFormat("Current Bid: $%.2f", items[selectedItemIndex].currentBid), GetScreenWidth() / 2 - MeasureText(TextFormat("Current Bid: $%.2f", items[selectedItemIndex].currentBid), 25) / 2, 140, 25, GREEN);

                    DrawInputBox(&bidAmountInput, "Bid Amount:");
                    DrawInputBox(&bidderNameInput, "Your Name:");

                    // Draw "BID!" button
                    Rectangle bidButtonRect = { GetScreenWidth() / 2 - 120, 480, 100, 40 };
                    DrawRectangleRec(bidButtonRect, GREEN_ACCEPT);
                    DrawRectangleLinesEx(bidButtonRect, 2, DARKGRAY_CUSTOM);
                    DrawText("BID!", bidButtonRect.x + bidButtonRect.width / 2 - MeasureText("BID!", 20) / 2, bidButtonRect.y + 10, 20, RAYWHITE);

                    // Draw "Cancel" button
                    Rectangle cancelButtonRect = { GetScreenWidth() / 2 + 20, 480, 100, 40 };
                    DrawRectangleRec(cancelButtonRect, RED_DECLINE);
                    DrawRectangleLinesEx(cancelButtonRect, 2, DARKGRAY_CUSTOM);
                    DrawText("Cancel", cancelButtonRect.x + cancelButtonRect.width / 2 - MeasureText("Cancel", 20) / 2, cancelButtonRect.y + 10, 20, RAYWHITE);

                } break; // End of SCREEN_PLACE_BID drawing
            }

            // Always draw temporary UI messages on top of everything else
            if (strlen(uiMessage) > 0) {
                DrawRectangle(0, GetScreenHeight() - 30, GetScreenWidth(), 30, YELLOW_WARNING);
                DrawText(uiMessage, GetScreenWidth() / 2 - MeasureText(uiMessage, 20) / 2, GetScreenHeight() - 25, 20, DARKGRAY);
            }

        EndDrawing(); // End drawing operations
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and release OpenGL context and Raylib resources
    //--------------------------------------------------------------------------------------

    return 0; // Indicate successful program execution
}

// --- Function Implementations ---

// InitAuctionData: Populates the 'items' array with some predefined auction data.
void InitAuctionData() {
    itemCount = 3; // We'll start with 3 items for demonstration purposes

    // Item 1: Antique Vase
    strcpy(items[0].name, "Antique Vase");
    strcpy(items[0].description, "A beautiful ceramic vase from the Ming Dynasty.");
    items[0].currentBid = 1500.00f;
    strcpy(items[0].highestBidder, "No Bids Yet"); // Initial state
    items[0].auctionClosed = false; // Auction is open

    // Item 2: Rare Comic Book
    strcpy(items[1].name, "Rare Comic Book");
    strcpy(items[1].description, "First edition of 'The Amazing Spider-Man #1'.");
    items[1].currentBid = 5000.00f;
    strcpy(items[1].highestBidder, "Peter P.");
    items[1].auctionClosed = false; // Auction is open

    // Item 3: Vintage Guitar (example of a closed auction)
    strcpy(items[2].name, "Vintage Guitar");
    strcpy(items[2].description, "1960s electric guitar, well-preserved.");
    items[2].currentBid = 2500.00f;
    strcpy(items[2].highestBidder, "Mary J.");
    items[2].auctionClosed = true; // Auction is closed
}

// DrawItemListItem: Renders a single auction item entry in the list view.
// It displays the item's name, current bid, and status.
void DrawItemListItem(int index, int x, int y, int width, int height) {
    Rectangle itemRect = { (float)x, (float)y, (float)width, (float)height };
    // Alternate row colors for better readability in the list
    Color bgColor = (index % 2 == 0) ? LIGHTGRAY_CUSTOM : RAYWHITE;

    // Change background color on mouse hover for visual feedback
    if (IsMouseOver(itemRect)) {
        bgColor = DARKGRAY_CUSTOM; // Darken on hover
    }

    DrawRectangleRec(itemRect, bgColor); // Draw the background rectangle
    DrawRectangleLinesEx(itemRect, 2, DARKGRAY_CUSTOM); // Draw the border

    // Draw item name on the left
    Color textColor = (IsMouseOver(itemRect)) ? RAYWHITE : BLACK; // Text color changes on hover
    DrawText(items[index].name, x + 10, y + 10, 20, textColor);
    // Format and draw current bid on the right
    char bidText[50]; // Buffer for formatted bid string
    snprintf(bidText, sizeof(bidText), "Current Bid: $%.2f", items[index].currentBid);
    DrawText(bidText, x + width - MeasureText(bidText, 20) - 10, y + 10, 20, textColor);

    // Draw auction status (OPEN/CLOSED) below the name
    Color statusColor = items[index].auctionClosed ? RED_DECLINE : GREEN_ACCEPT; // Red for closed, green for open
    DrawText(items[index].auctionClosed ? "CLOSED" : "OPEN", x + 10, y + 35, 15, statusColor);
}

// DrawInputBox: Renders a text input box on the screen, including its label.
void DrawInputBox(InputBox* box, const char* label) {
    DrawText(label, box->rect.x, box->rect.y - 25, 20, DARKGRAY_CUSTOM); // Draw label above the box
    DrawRectangleRec(box->rect, RAYWHITE); // Draw the input box background
    DrawRectangleLinesEx(box->rect, 2, box->borderColor); // Draw the border (color changes based on active state)

    // Draw masked text for passwords if isPassword is true
    if (box->isPassword) {
        char maskedText[MAX_INPUT_CHARS + 1];
        for (int i = 0; i < box->letterCount; i++) maskedText[i] = '*';
        maskedText[box->letterCount] = '\0'; // Null-terminate the masked string
        DrawText(maskedText, box->rect.x + 5, box->rect.y + 10, 20, BLACK);
    } else {
        DrawText(box->text, box->rect.x + 5, box->rect.y + 10, 20, BLACK); // Draw actual text for non-password fields
    }

    // Draw a blinking cursor if the input box is active
    if (box->active) {
        if (((int)(GetTime() * 2.0f) % 2) == 0) { // Simple blinking logic
            // Adjust cursor position for masked input as well
            float cursorX = box->rect.x + 5 + MeasureText(box->text, 20);
            if (box->isPassword) {
                char tempMasked[2] = {'*', '\0'};
                cursorX = box->rect.x + 5 + MeasureText(tempMasked, 20) * box->letterCount;
            }

            DrawText("_", (int)cursorX, (int)box->rect.y + 10, 20, BLACK); // Draw cursor
        }
    }
}

// UpdateInputBox: Handles keyboard input for an active input box and manages focus.
void UpdateInputBox(InputBox* box) {
    // Check if the mouse clicked on this input box to activate/deactivate it
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(GetMousePosition(), box->rect)) {
            box->active = true;
            box->borderColor = BLUE_HIGHLIGHT; // Highlight when active
        } else {
            box->active = false;
            box->borderColor = DARKGRAY_CUSTOM; // Revert to default when inactive
        }
    }

    // Only process keyboard input if this box is active
    if (box->active) {
        // Get character pressed (unicode)
        int key = GetCharPressed();
        while (key > 0) {
            // NOTE: Only allow characters in range [32..125] (keyboard)
            if (((key >= 32) && (key <= 125)) && (box->letterCount < MAX_INPUT_CHARS)) {
                box->text[box->letterCount] = (char)key;
                box->letterCount++;
            }
            key = GetCharPressed();  // Get next character if any
        }

        // Check for backspace (deletion)
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (box->letterCount > 0) {
                box->letterCount--;
                box->text[box->letterCount] = '\0'; // Null-terminate after deletion
            }
        }
        // If Enter is pressed, deactivate the box (often used to submit)
        if (IsKeyPressed(KEY_ENTER)) {
             box->active = false;
             box->borderColor = DARKGRAY_CUSTOM;
        }
    }
}

// IsMouseOver: A simple helper function to check if the mouse is over a rectangle.
bool IsMouseOver(Rectangle rect) {
    return CheckCollisionPointRec(GetMousePosition(), rect);
}

// ResetInputBoxes: Deactivates and clears all input boxes.
void ResetInputBoxes() {
    bidAmountInput.active = false;
    bidAmountInput.borderColor = DARKGRAY_CUSTOM;
    strcpy(bidAmountInput.text, "");
    bidAmountInput.letterCount = 0;

    bidderNameInput.active = false;
    bidderNameInput.borderColor = DARKGRAY_CUSTOM;
    strcpy(bidderNameInput.text, "");
    bidderNameInput.letterCount = 0;

    signInUsernameInput.active = false;
    signInUsernameInput.borderColor = DARKGRAY_CUSTOM;
    strcpy(signInUsernameInput.text, "");
    signInUsernameInput.letterCount = 0;

    signInPasswordInput.active = false;
    signInPasswordInput.borderColor = DARKGRAY_CUSTOM;
    strcpy(signInPasswordInput.text, "");
    signInPasswordInput.letterCount = 0;

    signUpUsernameInput.active = false;
    signUpUsernameInput.borderColor = DARKGRAY_CUSTOM;
    strcpy(signUpUsernameInput.text, "");
    signUpUsernameInput.letterCount = 0;

    signUpPasswordInput.active = false;
    signUpPasswordInput.borderColor = DARKGRAY_CUSTOM;
    strcpy(signUpPasswordInput.text, "");
    signUpPasswordInput.letterCount = 0;

    signUpConfirmPasswordInput.active = false;
    signUpConfirmPasswordInput.borderColor = DARKGRAY_CUSTOM;
    strcpy(signUpConfirmPasswordInput.text, "");
    signUpConfirmPasswordInput.letterCount = 0;
}

// SetUIMessage: Sets a temporary message to be displayed to the user.
void SetUIMessage(const char* message) {
    strcpy(uiMessage, message);
    uiMessageTimer = UI_MESSAGE_DURATION; // Start the timer for the message
}

// --- User Management Function Implementations ---

// HashPassword: A very simple (non-cryptographic) hash function for passwords.
// DO NOT use this for real-world applications! It's purely for demonstration.
unsigned int HashPassword(const char* password) {
    unsigned int hash = 5381; // djb2 hash algorithm initial value
    int c;
    while ((c = *password++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// LoadUsers: Reads user data from the USERS_FILE.
bool LoadUsers() {
    FILE* file = fopen(USERS_FILE, "r");
    if (file == NULL) {
        TraceLog(LOG_INFO, "USERS_FILE not found or unable to open for reading. Starting with no users.");
        return false; // File doesn't exist yet, or cannot be opened. No users loaded.
    }

    userCount = 0;
    while (userCount < MAX_USERS && fscanf(file, "%s %u", users[userCount].username, &users[userCount].hashedPassword) == 2) {
        userCount++;
    }

    fclose(file);
    TraceLog(LOG_INFO, "Loaded %d users from %s.", userCount, USERS_FILE);
    return true;
}

// SaveUsers: Writes current user data to the USERS_FILE.
bool SaveUsers() {
    FILE* file = fopen(USERS_FILE, "w"); // "w" truncates (clears) the file if it exists, then writes
    if (file == NULL) {
        TraceLog(LOG_WARNING, "Unable to open %s for writing. User data not saved.", USERS_FILE);
        return false;
    }

    for (int i = 0; i < userCount; i++) {
        fprintf(file, "%s %u\n", users[i].username, users[i].hashedPassword);
    }

    fclose(file);
    TraceLog(LOG_INFO, "Saved %d users to %s.", userCount, USERS_FILE);
    return true;
}

// RegisterUser: Adds a new user to the system.
bool RegisterUser(const char* username, const char* password) {
    if (userCount >= MAX_USERS) {
        SetUIMessage("Cannot register: maximum user limit reached.");
        return false;
    }
    if (UsernameExists(username)) {
        // This check should ideally be done before calling RegisterUser
        // but included here for robustness.
        SetUIMessage("Registration failed: Username already exists.");
        return false;
    }

    // Add new user
    strcpy(users[userCount].username, username);
    users[userCount].hashedPassword = HashPassword(password);
    userCount++;

    // Save users to file immediately after registration
    return SaveUsers();
}

// AuthenticateUser: Checks if provided username and password match a registered user.
bool AuthenticateUser(const char* username, const char* password) {
    unsigned int inputHash = HashPassword(password);
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0 && users[i].hashedPassword == inputHash) {
            return true; // Match found
        }
    }
    return false; // No match
}

// UsernameExists: Checks if a username is already taken.
bool UsernameExists(const char* username) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return true; // Username found
        }
    }
    return false; // Username not found
}
