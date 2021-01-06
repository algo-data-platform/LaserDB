module.exports = {
  docs: [ 
	{
		type : 'doc',
		id : 'about',
	},
	{
		type : 'doc',
		id : 'getting-started',
	},
	{
		type : 'doc',
		id : 'comparison-to-alternatives',
	},
	{
		type: 'category',
		label: '集群部署',
		items: [
			'cluster-deploy',
			'cluster-config',
		]
	},
	{
		type: 'category',
		label: '生态工具',
		items: [
			'laser-client',
			'laser-proxy',
			'laser-transform',
		]
	},
	{
		type: 'doc',
		id: 'monitor'
	},
	{
		type: 'category',
		label: '教程',
		items: [
			'doc1',
		]
	},
	{
		type: 'category',
		label: '参考指南',
		items: [
			'reference/data-model',
			'reference/data-shard',
			'reference/data-import',
			'reference/engine',
			'reference/server-config',
			'reference/server-error-code',
		]
	},
	{
		type: 'doc',
		id: 'fqa'
	},
	{
		type: 'doc',
		id: 'terminology'
	},
  ],
};
