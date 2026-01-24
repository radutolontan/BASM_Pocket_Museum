/**
 * Style Guide JavaScript
 * Handles theme switching and smooth scrolling
 */

// Theme Management
const themeToggle = document.getElementById('themeToggle');
const body = document.body;

// Load saved theme from localStorage
const savedTheme = localStorage.getItem('theme') || 'light';
body.setAttribute('data-theme', savedTheme);
updateThemeButton(savedTheme);

// Theme toggle event listener
themeToggle.addEventListener('click', () => {
    const currentTheme = body.getAttribute('data-theme');
    const newTheme = currentTheme === 'light' ? 'dark' : 'light';

    body.setAttribute('data-theme', newTheme);
    localStorage.setItem('theme', newTheme);
    updateThemeButton(newTheme);
});

/**
 * Update theme button text and icon
 */
function updateThemeButton(theme) {
    const icon = themeToggle.querySelector('i');
    const text = themeToggle.querySelector('span');

    if (theme === 'dark') {
        icon.className = 'bi bi-sun-fill';
        text.textContent = 'Light Theme';
    } else {
        icon.className = 'bi bi-moon-fill';
        text.textContent = 'Dark Theme';
    }
}

// Smooth scrolling for anchor links
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
        e.preventDefault();
        const target = document.querySelector(this.getAttribute('href'));
        if (target) {
            target.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });
        }
    });
});

// Add copy-to-clipboard for color hex codes
document.querySelectorAll('.color-hex').forEach(hexElement => {
    hexElement.style.cursor = 'pointer';
    hexElement.title = 'Click to copy';

    hexElement.addEventListener('click', function() {
        const hexCode = this.textContent;
        navigator.clipboard.writeText(hexCode).then(() => {
            const originalText = this.textContent;
            this.textContent = 'Copied!';
            this.style.color = '#10b981';

            setTimeout(() => {
                this.textContent = originalText;
                this.style.color = '';
            }, 1500);
        }).catch(err => {
            console.error('Failed to copy:', err);
        });
    });
});

// Log theme on load
console.log(`Style Guide loaded with ${savedTheme} theme`);
