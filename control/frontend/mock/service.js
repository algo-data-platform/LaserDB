import Mock from 'mockjs'

const data = Mock.mock({
  'items|30': [{
    'id|+1': 1000,
    name: '@sentence(1, 2)',
    rule: '@sentence(2, 3)',
    databaseName: '@sentence(1, 1)',
    tableName: '@sentence(1, 1)',
    desc: '@sentence(10, 20)'
  }]
})

export default [
  {
    url: '/service/list',
    type: 'get',
    response: config => {
      const items = data.items
      return {
        code: 20000,
        data: {
          total: items.length,
          items: items
        }
      }
    }
  }
]
