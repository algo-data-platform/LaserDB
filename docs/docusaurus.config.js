module.exports = {
  title: 'LaserDB',
  tagline: 'A distributed database with high performance which support batch import',
  url: 'https://algo-data-platform.github.io',
  baseUrl: '/LaserDB/',
  onBrokenLinks: 'throw',
  favicon: 'img/small-logo.png',
  organizationName: 'algo-data-platform', // Usually your GitHub org/user name.
  projectName: 'LaserDB', // Usually your repo name.
  themeConfig: {
    colorMode: {
      disableSwitch: true,
    },
    navbar: {
      title: '',
      logo: {
        alt: 'My Site Logo',
        src: 'img/logo.png',
      },
      items: [
        {
          type: 'docsVersionDropdown',
          position: 'right',
          dropdownActiveClassDisabled: true,
          dropdownItemsBefore: [],
        },
        {
          to: 'docs/',
          activeBasePath: 'docs',
          label: 'Docs',
          position: 'right',
        },
        { to: 'blog', label: 'Blog', position: 'right' },
        {
          href: 'https://github.com/algo-data-platform/LaserDB',
          label: 'GitHub',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: '文档',
          items: [
            {
              label: '关于 LaserDB',
              to: 'docs/',
            },
            {
              label: '快速上手',
              to: 'docs/getting-started',
            },
            {
              label: '参考指南',
              to: 'docs/reference/data-model',
            },
          ],
        },
        {
          title: 'Community',
          items: [
            {
              label: 'Stack Overflow',
              href: 'https://stackoverflow.com/questions/tagged/docusaurus',
            },
            {
              label: 'Discord',
              href: 'https://discordapp.com/invite/docusaurus',
            },
            {
              label: 'Twitter',
              href: 'https://twitter.com/docusaurus',
            },
          ],
        },
        {
          title: 'More',
          items: [
            {
              label: 'Blog',
              to: 'blog',
            },
            {
              label: 'GitHub',
              href: 'https://github.com/algo-data-platform/LaserDB',
            },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} LaserDB.`,
    },
  },
  presets: [
    [
      '@docusaurus/preset-classic',
      {
        docs: {
          sidebarPath: require.resolve('./sidebars.js'),
          // Please change this to your repo.
          editUrl:
            'https://github.com/algo-data-platform/LaserDB/edit/main/docs/',
        },
        blog: {
          showReadingTime: true,
          // Please change this to your repo.
          editUrl:
            'https://github.com/algo-data-platform/LaserDB/edit/main/docs/blog/',
        },
        theme: {
          customCss: require.resolve('./src/css/custom.css'),
        },
      },
    ],
  ],
};
