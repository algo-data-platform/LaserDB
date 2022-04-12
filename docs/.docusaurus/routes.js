
import React from 'react';
import ComponentCreator from '@docusaurus/ComponentCreator';
export default [
{
  path: '/LaserDB/',
  component: ComponentCreator('/LaserDB/','3e9'),
  exact: true,
},
{
  path: '/LaserDB/blog',
  component: ComponentCreator('/LaserDB/blog','666'),
  exact: true,
},
{
  path: '/LaserDB/blog/laserdb-in-weibo',
  component: ComponentCreator('/LaserDB/blog/laserdb-in-weibo','d3a'),
  exact: true,
},
{
  path: '/LaserDB/blog/tags',
  component: ComponentCreator('/LaserDB/blog/tags','0f3'),
  exact: true,
},
{
  path: '/LaserDB/blog/tags/laser-db',
  component: ComponentCreator('/LaserDB/blog/tags/laser-db','eb3'),
  exact: true,
},
{
  path: '/LaserDB/blog/tags/weibo',
  component: ComponentCreator('/LaserDB/blog/tags/weibo','98c'),
  exact: true,
},
{
  path: '/LaserDB/docs',
  component: ComponentCreator('/LaserDB/docs','38b'),
  
  routes: [
{
  path: '/LaserDB/docs/',
  component: ComponentCreator('/LaserDB/docs/','add'),
  exact: true,
},
{
  path: '/LaserDB/docs/cluster-config',
  component: ComponentCreator('/LaserDB/docs/cluster-config','289'),
  exact: true,
},
{
  path: '/LaserDB/docs/cluster-deploy',
  component: ComponentCreator('/LaserDB/docs/cluster-deploy','e5b'),
  exact: true,
},
{
  path: '/LaserDB/docs/comparison-to-alternatives',
  component: ComponentCreator('/LaserDB/docs/comparison-to-alternatives','a9b'),
  exact: true,
},
{
  path: '/LaserDB/docs/doc2',
  component: ComponentCreator('/LaserDB/docs/doc2','ddc'),
  exact: true,
},
{
  path: '/LaserDB/docs/doc3',
  component: ComponentCreator('/LaserDB/docs/doc3','17a'),
  exact: true,
},
{
  path: '/LaserDB/docs/fqa',
  component: ComponentCreator('/LaserDB/docs/fqa','72b'),
  exact: true,
},
{
  path: '/LaserDB/docs/getting-started',
  component: ComponentCreator('/LaserDB/docs/getting-started','8c7'),
  exact: true,
},
{
  path: '/LaserDB/docs/laser-client',
  component: ComponentCreator('/LaserDB/docs/laser-client','016'),
  exact: true,
},
{
  path: '/LaserDB/docs/laser-proxy',
  component: ComponentCreator('/LaserDB/docs/laser-proxy','b7d'),
  exact: true,
},
{
  path: '/LaserDB/docs/laser-transform',
  component: ComponentCreator('/LaserDB/docs/laser-transform','1fa'),
  exact: true,
},
{
  path: '/LaserDB/docs/mdx',
  component: ComponentCreator('/LaserDB/docs/mdx','11a'),
  exact: true,
},
{
  path: '/LaserDB/docs/monitor',
  component: ComponentCreator('/LaserDB/docs/monitor','f50'),
  exact: true,
},
{
  path: '/LaserDB/docs/reference/data-import',
  component: ComponentCreator('/LaserDB/docs/reference/data-import','774'),
  exact: true,
},
{
  path: '/LaserDB/docs/reference/data-model',
  component: ComponentCreator('/LaserDB/docs/reference/data-model','b53'),
  exact: true,
},
{
  path: '/LaserDB/docs/reference/data-shard',
  component: ComponentCreator('/LaserDB/docs/reference/data-shard','2fb'),
  exact: true,
},
{
  path: '/LaserDB/docs/reference/engine',
  component: ComponentCreator('/LaserDB/docs/reference/engine','769'),
  exact: true,
},
{
  path: '/LaserDB/docs/reference/server-config',
  component: ComponentCreator('/LaserDB/docs/reference/server-config','aea'),
  exact: true,
},
{
  path: '/LaserDB/docs/reference/server-error-code',
  component: ComponentCreator('/LaserDB/docs/reference/server-error-code','702'),
  exact: true,
},
{
  path: '/LaserDB/docs/reference/server-metrics',
  component: ComponentCreator('/LaserDB/docs/reference/server-metrics','4c0'),
  exact: true,
},
{
  path: '/LaserDB/docs/terminology',
  component: ComponentCreator('/LaserDB/docs/terminology','e00'),
  exact: true,
},
{
  path: '/LaserDB/docs/xxx',
  component: ComponentCreator('/LaserDB/docs/xxx','8fd'),
  exact: true,
},
]
},
{
  path: '*',
  component: ComponentCreator('*')
}
];
