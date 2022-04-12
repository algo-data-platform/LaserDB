export default {
  "title": "LaserDB",
  "tagline": "A distributed database with high performance which support batch import",
  "url": "https://algo-data-platform.github.io",
  "baseUrl": "/LaserDB/",
  "onBrokenLinks": "throw",
  "favicon": "img/small-logo.png",
  "organizationName": "algo-data-platform",
  "projectName": "LaserDB",
  "themeConfig": {
    "colorMode": {
      "disableSwitch": true,
      "defaultMode": "light",
      "respectPrefersColorScheme": false,
      "switchConfig": {
        "darkIcon": "🌜",
        "darkIconStyle": {},
        "lightIcon": "🌞",
        "lightIconStyle": {}
      }
    },
    "navbar": {
      "title": "",
      "logo": {
        "alt": "My Site Logo",
        "src": "img/logo.png"
      },
      "items": [
        {
          "type": "docsVersionDropdown",
          "position": "right",
          "dropdownActiveClassDisabled": true,
          "dropdownItemsBefore": [],
          "dropdownItemsAfter": []
        },
        {
          "to": "docs/",
          "activeBasePath": "docs",
          "label": "Docs",
          "position": "right"
        },
        {
          "to": "blog",
          "label": "Blog",
          "position": "right"
        },
        {
          "href": "https://github.com/algo-data-platform/LaserDB",
          "label": "GitHub",
          "position": "right"
        }
      ],
      "hideOnScroll": false
    },
    "footer": {
      "style": "dark",
      "links": [
        {
          "title": "文档",
          "items": [
            {
              "label": "关于 LaserDB",
              "to": "docs/"
            },
            {
              "label": "快速上手",
              "to": "docs/getting-started"
            },
            {
              "label": "参考指南",
              "to": "docs/reference/data-model"
            }
          ]
        },
        {
          "title": "Community",
          "items": [
            {
              "label": "Stack Overflow",
              "href": "https://stackoverflow.com/questions/tagged/docusaurus"
            },
            {
              "label": "Discord",
              "href": "https://discordapp.com/invite/docusaurus"
            },
            {
              "label": "Twitter",
              "href": "https://twitter.com/docusaurus"
            }
          ]
        },
        {
          "title": "More",
          "items": [
            {
              "label": "Blog",
              "to": "blog"
            },
            {
              "label": "GitHub",
              "href": "https://github.com/algo-data-platform/LaserDB"
            }
          ]
        }
      ],
      "copyright": "Copyright © 2022 LaserDB."
    },
    "docs": {
      "versionPersistence": "localStorage"
    },
    "metadatas": [],
    "prism": {
      "additionalLanguages": []
    }
  },
  "presets": [
    [
      "@docusaurus/preset-classic",
      {
        "docs": {
          "sidebarPath": "/Volumes/workspace/github/liubang/LaserDB/docs/sidebars.js",
          "editUrl": "https://github.com/algo-data-platform/LaserDB/edit/main/docs/"
        },
        "blog": {
          "showReadingTime": true,
          "editUrl": "https://github.com/algo-data-platform/LaserDB/edit/main/docs/blog/"
        },
        "theme": {
          "customCss": "/Volumes/workspace/github/liubang/LaserDB/docs/src/css/custom.css"
        }
      }
    ]
  ],
  "onDuplicateRoutes": "warn",
  "customFields": {},
  "plugins": [],
  "themes": [],
  "titleDelimiter": "|",
  "noIndex": false
};